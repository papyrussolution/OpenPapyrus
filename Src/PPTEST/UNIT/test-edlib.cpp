// TEST-EDLIB.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// The author of the original and the EdLib: Martin Sosic
//
#include <pp.h>
#pragma hdrstop
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <algorithm>
#include <vector>

#include "..\slib\edlib\edlib.h"
//#include "..\slib\edlib\SimpleEditDistance.h"

static int CalcEditDistanceSimple(const char* query, int queryLength, const char* target, int targetLength, const EdlibAlignMode mode, int* score, int** positions_, int* numPositions_) 
{
	int bestScore = -1;
	std::vector <int> positions;
	// Handle as a special situation when one of the sequences has length 0.
	if(queryLength == 0 || targetLength == 0) {
		if(mode == EDLIB_MODE_NW) {
			*score = std::max(queryLength, targetLength);
			*positions_ = new int[1];
			*positions_[0] = targetLength - 1;
			*numPositions_ = 1;
		}
		else if(mode == EDLIB_MODE_SHW || mode == EDLIB_MODE_HW) {
			*score = queryLength;
			*positions_ = new int[1];
			*positions_[0] = -1;
			*numPositions_ = 1;
		}
		else {
			return EDLIB_STATUS_ERROR;
		}
		return EDLIB_STATUS_OK;
	}
	int * C = new int[queryLength];
	int * newC = new int[queryLength];
	// set first column (column zero)
	for(int i = 0; i < queryLength; i++) {
		C[i] = i + 1;
	}
	/*
	   for(int i = 0; i < queryLength; i++)
	    printf("%3d ", C[i]);
	   printf("\n");
	 */
	for(int c = 0; c < targetLength; c++) { // for each column
		newC[0] = smin3((mode == EDLIB_MODE_HW ? 0 : c + 1) + 1, // up
			(mode == EDLIB_MODE_HW ? 0 : c) + (target[c] == query[0] ? 0 : 1), // up left
			C[0] + 1); // left
		for(int r = 1; r < queryLength; r++) {
			newC[r] = smin3(newC[r-1] + 1, // up
				C[r-1] + (target[c] == query[r] ? 0 : 1), // up left
				C[r] + 1); // left
		}
		/*  for (int i = 0; i < queryLength; i++)
		    printf("%3d ", newC[i]);
		    printf("\n");*/

		if(mode != EDLIB_MODE_NW || c == targetLength - 1) { // For NW check only last column
			int newScore = newC[queryLength - 1];
			if(bestScore == -1 || newScore <= bestScore) {
				if(newScore < bestScore) {
					positions.clear();
				}
				bestScore = newScore;
				positions.push_back(c);
			}
		}
		int * tmp = C;
		C = newC;
		newC = tmp;
	}
	delete[] C;
	delete[] newC;
	*score = bestScore;
	if(positions.size() > 0) {
		*positions_ = new int[positions.size()];
		*numPositions_ = static_cast<int>(positions.size());
		copy(positions.begin(), positions.end(), *positions_);
	}
	else {
		*positions_ = NULL;
		*numPositions_ = 0;
	}
	return EDLIB_STATUS_OK;
}
// 
// Checks if alignment is correct.
// 
bool checkAlignment(const char* query, int queryLength, const char* target, int score, int pos, EdlibAlignMode mode, unsigned char* alignment, int alignmentLength) 
{
	int alignScore = 0;
	int qIdx = queryLength - 1;
	int tIdx = pos;
	for(int i = alignmentLength - 1; i >= 0; i--) {
		if(alignment[i] == EDLIB_EDOP_MATCH) { // match
			if(query[qIdx] != target[tIdx]) {
				printf("Should be match but is a mismatch! (tIdx, qIdx, i): (%d, %d, %d)\n", tIdx, qIdx, i);
				return false;
			}
			qIdx--;
			tIdx--;
		}
		else if(alignment[i] == EDLIB_EDOP_MISMATCH) { // mismatch
			if(query[qIdx] == target[tIdx]) {
				printf("Should be mismatch but is a match! (tIdx, qIdx, i): (%d, %d, %d)\n", tIdx, qIdx, i);
				return false;
			}
			alignScore += 1;
			qIdx--;
			tIdx--;
		}
		else if(alignment[i] == EDLIB_EDOP_INSERT) {
			alignScore += 1;
			qIdx--;
		}
		else if(alignment[i] == EDLIB_EDOP_DELETE) {
			if(!(mode == EDLIB_MODE_HW && qIdx == -1))
				alignScore += 1;
			tIdx--;
		}
		if(tIdx < -1 || qIdx < -1) {
			printf("Alignment went outside of matrix! (tIdx, qIdx, i): (%d, %d, %d)\n", tIdx, qIdx, i);
			return false;
		}
	}
	if(qIdx != -1) {
		printf("Alignment did not reach end!\n");
		return false;
	}
	if(alignScore != score) {
		printf("Wrong score in alignment! %d should be %d\n", alignScore, score);
		return false;
	}
	if(alignmentLength > 0 && alignment[0] == EDLIB_EDOP_INSERT && tIdx >= 0) {
		printf("Alignment starts with insertion in target, while it could start with mismatch!\n");
		return false;
	}
	return true;
}
// 
// @param alignment
// @param alignmentLength
// @param endLocation
// @return Return start location of alignment in target, if there is none return -1.
//
static int getAlignmentStart(const uchar * alignment, int alignmentLength, int endLocation) 
{
	int start_location = endLocation + 1;
	for(int i = 0; i < alignmentLength; i++) {
		if(alignment[i] != EDLIB_EDOP_INSERT)
			start_location--;
	}
	return (start_location > endLocation) ? -1 : start_location;
}

static bool executeTest(const char * query, int queryLength, const char * target, int targetLength, EdlibAlignMode mode) 
{
	printf(mode == EDLIB_MODE_HW ? "HW:  " : mode == EDLIB_MODE_SHW ? "SHW: " : "NW:  ");
	bool pass = true;
	int scoreSimple = -1; 
	int numLocationsSimple = 0; 
	int * endLocationsSimple = NULL;
	CalcEditDistanceSimple(query, queryLength, target, targetLength, mode, &scoreSimple, &endLocationsSimple, &numLocationsSimple);
	EdlibAlignResult result = edlibAlign(query, queryLength, target, targetLength, edlibNewAlignConfig(-1, mode, EDLIB_TASK_PATH, NULL, 0));
	if(result.editDistance != scoreSimple) {
		pass = false;
		printf("Scores: expected %d, got %d\n", scoreSimple, result.editDistance);
	}
	else if(result.numLocations != numLocationsSimple) {
		pass = false;
		printf("Number of locations: expected %d, got %d\n", numLocationsSimple, result.numLocations);
	}
	else {
		for(int i = 0; i < result.numLocations; i++) {
			if(result.endLocations[i] != endLocationsSimple[i]) {
				pass = false;
				printf("End locations at %d are not equal! Expected %d, got %d\n", i, endLocationsSimple[i], result.endLocations[1]);
				break;
			}
		}
	}
	if(result.alignment) {
		if(!checkAlignment(query, queryLength, target,
		    result.editDistance, result.endLocations[0], mode,
		    result.alignment, result.alignmentLength)) {
			pass = false;
			printf("Alignment is not correct\n");
		}
		int alignmentStart = getAlignmentStart(result.alignment, result.alignmentLength, result.endLocations[0]);
		if(result.startLocations[0] != alignmentStart) {
			pass = false;
			printf("Start location (%d) is not consistent with alignment start (%d)\n", result.startLocations[0], alignmentStart);
		}
	}
	printf(pass ? "\x1B[32m OK \x1B[0m\n" : "\x1B[31m FAIL \x1B[0m\n");
	delete [] endLocationsSimple;
	edlibFreeAlignResult(result);
	return pass;
}

void FillRandomly(char* seq, int seqLength, int alphabetLength) 
{
	for(int i = 0; i < seqLength; i++)
		seq[i] = static_cast<char>(rand()) % alphabetLength;
}

// Returns true if all tests passed, false otherwise.
static bool RunRandomTests(int numTests, EdlibAlignMode mode, bool findAlignment) 
{
	int alphabetLength = 10;
	int numTestsFailed = 0;
	clock_t start;
	double timeEdlib = 0;
	double timeSimple = 0;
	for(int i = 0; i < numTests; i++) {
		bool failed = false;
		int queryLength = 50 + rand() % 300;
		int targetLength = 500 + rand() % 10000;
		char * query = static_cast<char *>(SAlloc::M(sizeof(char) * queryLength));
		char * target = static_cast<char *>(SAlloc::M(sizeof(char) * targetLength));
		FillRandomly(query, queryLength, alphabetLength);
		FillRandomly(target, targetLength, alphabetLength);

		// // Print query
		// printf("Query: ");
		// for (int i = 0; i < queryLength; i++)
		//     printf("%d", query[i]);
		// printf("\n");

		// // Print target
		// printf("Target: ");
		// for (int i = 0; i < targetLength; i++)
		//     printf("%d", target[i]);
		// printf("\n");

		start = clock();
		EdlibAlignResult result = edlibAlign(query, queryLength, target, targetLength,
			edlibNewAlignConfig(-1, mode, findAlignment ? EDLIB_TASK_PATH : EDLIB_TASK_DISTANCE, NULL, 0));
		timeEdlib += clock() - start;
		if(result.alignment) {
			if(!checkAlignment(query, queryLength, target,
			    result.editDistance, result.endLocations[0], mode,
			    result.alignment, result.alignmentLength)) {
				failed = true;
				printf("Alignment is not correct\n");
			}
			int alignmentStart = getAlignmentStart(result.alignment, result.alignmentLength,
				result.endLocations[0]);
			if(result.startLocations[0] != alignmentStart) {
				failed = true;
				printf("Start location (%d) is not consistent with alignment start (%d)\n", result.startLocations[0], alignmentStart);
			}
		}
		start = clock();
		int score2; int numLocations2;
		int* endLocations2;
		CalcEditDistanceSimple(query, queryLength, target, targetLength, mode, &score2, &endLocations2, &numLocations2);
		timeSimple += clock() - start;

		// Compare results
		if(result.editDistance != score2) {
			failed = true;
			printf("Scores are different! Expected %d, got %d)\n", score2, result.editDistance);
		}
		else if(result.editDistance == -1 && !(result.endLocations == NULL)) {
			failed = true;
			printf("Score was not found but endLocations is not NULL!\n");
		}
		else if(result.numLocations != numLocations2) {
			failed = true;
			printf("Number of endLocations returned is not equal! Expected %d, got %d\n",
			    numLocations2, result.numLocations);
		}
		else {
			for(int j = 0; j < result.numLocations; j++) {
				if(result.endLocations[j] != endLocations2[j]) {
					failed = true;
					printf("EndLocations at %d are not equal! Expected %d, got %d\n",
					    j, endLocations2[j], result.endLocations[j]);
					break;
				}
			}
		}
		edlibFreeAlignResult(result);
		delete [] endLocations2;
		for(int k = smax(score2 - 1, 0); k <= score2 + 1; k++) {
			int scoreExpected = score2 > k ? -1 : score2;
			EdlibAlignResult result3 = edlibAlign(query, queryLength, target, targetLength,
				edlibNewAlignConfig(k, mode, findAlignment ? EDLIB_TASK_PATH : EDLIB_TASK_DISTANCE, NULL, 0));
			if(result3.editDistance != scoreExpected) {
				failed = true;
				printf("For k = %d score was %d but it should have been %d\n", k, result3.editDistance, scoreExpected);
			}
			if(result3.alignment) {
				if(!checkAlignment(query, queryLength, target,
				    result3.editDistance, result3.endLocations[0],
				    mode, result3.alignment, result3.alignmentLength)) {
					failed = true;
					printf("Alignment is not correct\n");
				}
				int alignmentStart = getAlignmentStart(result3.alignment, result3.alignmentLength,
					result3.endLocations[0]);
				if(result3.startLocations[0] != alignmentStart) {
					failed = true;
					printf("Start location (%d) is not consistent with alignment start (%d)\n",
					    result3.startLocations[0], alignmentStart);
				}
			}
			edlibFreeAlignResult(result3);
		}

		if(failed)
			numTestsFailed++;
		SAlloc::F(query);
		SAlloc::F(target);
	}

	printf(mode == EDLIB_MODE_HW ? "HW: " : mode == EDLIB_MODE_SHW ? "SHW: " : "NW: ");
	printf(numTestsFailed == 0 ? "\x1B[32m" : "\x1B[31m");
	printf("%d/%d", numTests - numTestsFailed, numTests);
	printf("\x1B[0m");
	printf(" random tests passed!\n");
	double mTime = static_cast<double>(timeEdlib)/CLOCKS_PER_SEC;
	double sTime = static_cast<double>(timeSimple)/CLOCKS_PER_SEC;
	printf("Time Edlib: %lf\n", mTime);
	printf("Time Simple: %lf\n", sTime);
	printf("Times faster: %.2lf\n", sTime / mTime);
	return numTestsFailed == 0;
}

SLTEST_R(EdLib)
{
	bool local_ok = false;
	{ // bool test1() 
		const char query[] = {0, 1, 2, 3};
		const char target[] = {0, 1, 2, 3};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test2() 
		const const char query[] = {0, 1, 2, 3, 4}; // "match"
		const const char target[] = {8, 5, 0, 1, 3, 4, 6, 7, 5}; // "remachine"
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test3() 
		const char query[] = {0, 1, 2, 3, 4};
		const char target[] = {1, 2, 0, 1, 2, 3, 4, 5, 4};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test4() 
		const char query[200/*don't skip explicit size*/] = {0};
		const char target[200/*don't skip explicit size*/] = {1};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test5() 
		// Testing for special case when queryLength == word size
		const char query[64/*don't skip explicit size*/] = {0};
		const char target[64/*don't skip explicit size*/] = {1};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test6() 
		// Testing for special case when queryLength == word size
		const char query[] = {1, 3, 0, 1, 1, 1, 3, 0, 1, 3, 1, 3, 3};
		const char target[] = {0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3,
				3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3,
				1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1,
				3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3,
				3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0,
				1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2,
				3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3,
				0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3,
				2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1,
				3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2,
				3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0,
				1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2,
				2, 3, 2, 3, 3, 1, 0, 1, 1, 1, 0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1, 0, 1, 1, 1,
				0, 1, 3, 0, 1, 3, 3, 3, 1, 3, 2, 2, 3, 2, 3, 3, 1};

		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test7() 
		const char query[] = {2, 3, 0};
		const char target[] = {0, 1, 2, 2, 0};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test8() 
		const char query[] = {2, 3, 0};
		const char target[] = {2, 2, 0};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test9() 
		const char query[] = {9, 5, 5, 9, 9, 4, 6, 0, 1, 1, 5, 4, 6, 0, 6, 5, 5, 6, 5, 2, 2, 0, 6, 0, 8, 3, 7, 0, 6, 6, 4, 8, 3, 1, 9, 4, 5, 5, 5, 7, 8, 2,
			 3, 6, 4, 1, 1, 2, 7, 7, 6, 0, 9, 2, 0, 9, 6, 9, 9, 4, 6, 5, 2, 9};
		const char target[] = {7, 1, 6, 2, 9, 1, 1, 7, 5, 5, 4, 9, 6, 7, 3, 4, 6, 9, 4, 5, 2, 6, 6, 0, 7, 8, 4, 3, 3, 9, 5, 2, 0, 1, 7, 1, 4, 0, 9, 9, 7,
			 5, 0, 6, 2, 4, 0, 9, 3, 6, 6, 7, 4, 3, 9, 3, 3, 4, 7, 8, 5, 4, 1, 7, 7, 0, 9, 3, 0, 8, 4, 0, 3, 4, 6, 7, 0, 8,
			 6, 6, 6, 5,
			 5, 2, 0, 5, 5, 3, 1, 4, 1, 6, 8, 4, 3, 7, 6, 2, 0, 9, 0, 4, 9, 5, 1, 5, 3, 1, 3, 1, 9, 9, 6, 5, 1, 8, 0, 6, 1,
			 1, 1, 5, 9,
			 1, 1, 2, 1, 8, 5, 1, 7, 7, 8, 6, 5, 9, 1, 0, 2, 4, 1, 2, 5, 0, 9, 6, 8, 1, 4, 2, 4, 5, 9, 3, 9, 0, 5, 0, 8, 0,
			 3, 7, 0, 1,
			 3, 5, 0, 6, 5, 5, 2, 8, 9, 7, 0, 8, 5, 1, 9, 0, 3, 3, 7, 2, 6, 6, 4, 3, 8, 5, 6, 2, 2, 6, 5, 8, 3, 8, 4, 0, 3,
			 7, 8, 2, 6,
			 9, 0, 2, 0, 1, 2, 5, 6, 1, 9, 4, 8, 3, 7, 8, 8, 5, 2, 3, 1, 8, 1, 6, 6, 7, 6, 9, 6, 5, 3, 3, 6, 5, 7, 8, 6, 1,
			 3, 4, 2, 4,
			 0, 0, 7, 7, 1, 8, 5, 3, 3, 6, 1, 4, 5, 7, 3, 1, 8, 0, 8, 1, 5, 6, 6, 2, 4, 4, 3, 9, 8, 7, 3, 8, 0, 3, 8, 1, 3,
			 3, 4, 6, 1,
			 8, 2, 6, 7, 5, 8, 6, 7, 8, 7, 4, 5, 6, 6, 9, 0, 1, 1, 1, 9, 4, 9, 1, 9, 9, 2, 2, 4, 8, 0, 6, 6, 4, 4, 4, 2, 2,
			 2, 9, 3, 1,
			 6, 8, 7, 2, 9, 8, 6, 0, 1, 7, 7, 2, 8, 6, 2, 2, 1, 6, 0, 3, 4, 9, 8, 9, 3, 2, 3, 5, 3, 6, 6, 9, 6, 6, 2, 6, 6,
			 0, 8, 7, 9,
			 5, 9, 7, 4, 3, 1, 7, 2, 1, 0, 6, 0, 0, 7, 5, 2, 1, 2, 6, 9, 1, 5, 6, 7};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test10() 
		const char query[] = {0, 1, 2};
		const char target[] = {1, 1, 1};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	// Check if edlib works for whole range of char values.
	{ // bool test11() 
		// NOTE(Martin): I am using CHAR_MIN and CHAR_MAX because 'char' type is not guaranteed to be
		//   signed or unsigned by compiler, we can't know if it is signed or unsigned.
		const char query[] =  {CHAR_MIN, CHAR_MIN + (CHAR_MAX - CHAR_MIN) / 2, CHAR_MAX};
		const char target[] = {CHAR_MIN, CHAR_MIN + (CHAR_MAX - CHAR_MIN) / 2 + 1, CHAR_MAX};
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(query, SIZEOFARRAY(query), target, SIZEOFARRAY(target), EDLIB_MODE_SHW));
	}
	{ // bool test12() 
		const EdlibEqualityPair additionalEqualities[24] = {{'R', 'A'}, {'R', 'G'}, {'M', 'A'}, {'M', 'C'}, {'W', 'A'}, {'W', 'T'},
			{'S', 'C'}, {'S', 'G'}, {'Y', 'C'}, {'Y', 'T'}, {'K', 'G'}, {'K', 'T'},
			{'V', 'A'}, {'V', 'C'}, {'V', 'G'}, {'H', 'A'}, {'H', 'C'}, {'H', 'T'},
			{'D', 'A'}, {'D', 'G'}, {'D', 'T'}, {'B', 'C'}, {'B', 'G'}, {'B', 'T'}};
		const char * query = "GCATATCAATAAGCGGAGGA";
		const char * target =
			"TAACAAGGTTTCCGTAGGTGAACCTGCGGAAGGATCATTATCGAATAAACTTGATGGGTTGTCGCTGGCTTCTAGGAGCATGTGCACATCCGTCATTTTTATCCATCCACCTGTGCACCTTTTGTAGTCTTTGGAGGTAATAAGCGTGAATCTATCGAGGTCCTCTGGTCCTCGGAAAGAGGTGTTTGCCATATGGCTCGCCTTTGATACTCGCGAGTTACTCTAAGACTATGTCCTTTCATATACTACGAATGTAATAGAATGTATTCATTGGGCCTCAGTGCCTATAAAACATATACAACTTTCAGCAACGGATCTCTTGGCTCTCGCATCGATGAAGAACGCAGCGAAATGCGATAAGTAATGTGAATTGCAGAATTCAGTGAATCATCGAATCTTTGAACGCACCTTGCGCTCCTTGGTATTCCGAGGAGCATGCCTGTTTGAGTGTCATTAAATTCTCAACCCCTTCCGGTTTTTTGACTGGCTTTGGGGCTTGGATGTGGGGGATTCATTTGCGGGCCTCTGTAGAGGTCGGCTCCCCTGAAATGCATTAGTGGAACCGTTTGCGGTTACCGTCGCTGGTGTGATAACTATCTATGCCAAAGACAAACTGCTCTCTGATAGTTCTGCTTCTAACCGTCCATTTATTGGACAACATTATTATGAACACTTGACCTCAAATCAGGTAGGACTACCCGCTGAACTTAAGCATATCAATAAGCGGAGGAAAAGAAACTAACAAGGATTCCCCTAGTAACTGCGAGTGAAGCGGGAAAAGCTCAAATTTAAAATCTGGCGGTCTTTGGCCGTCCGAGTTGTAATCTAGAGAAGCGACACCCGCGCTGGACCGTGTACAAGTCTCCTGGAATGGAGCGTCATAGAGGGTGAGAATCCCGTCTCTGACACGGACTACCAGGGCTTTGTGGTGCGCTCTCAAAGAGTCGAGTTGTTTGGGAATGCAGCTCTAAATGGGTGGTAAATTCCATCTAAAGCTAAATATTGGCGAGAGACCGATAGCGAACAAGTACCGTGAGGGAAAGATGAAAAGAACTTTGGAAAGAGAGTTAAACAGTACGTGAAATTGCTGAAAGGGAAACGCTTGAAGTCAGTCGCGTTGGCCGGGGATCAGCCTCGCTTTTGCGTGGTGTATTTCCTGGTTGACGGGTCAGCATCAATTTTGACCGCTGGAAAAGGACTTGGGGAATGTGGCATCTTCGGATGTGTTATAGCCCTTTGTCGCATACGGCGGTTGGGATTGAGGAACTCAGCACGCCGCAAGGCCGGGTTTCGACCACGTTCGTGCTTAGGATGCTGGCATAATGGCTTTAATCGACCCGTCTTGAAACACGGACCAAGGAGTCTAACATGCCTGCGAGTGTTTGGGTGGAAAACCCGAGCGCGTAATGAAAGTGAAAGTTGAGATCCCTGTCGTGGGGAGCATCGACGCCCGGACCAGAACTTTTGGGACGGATCTGCGGTAGAGCATGTATGTTGGGACCCGAAAGATGGTGAACTATGCCTGAATAGGGTGAAGCCAGAGGAAACTCTGGTGGAGGCTCGTAGCGATTCTGACGTGCAAATCGATCGTCAAATTTGGGTATAGGGGCGAAAGACTAATCGAACCATCTAGTAGCTGGTTCCTGCCGAAGTTTCCCTCAGGATAGCAGAAACTCATATCAGATTTATGTGGTAAAGCGAATGATTAGAGGCCTTGGGGTTGAAACAACCTTAACCTATTCTCAAACTTTAAATATGTAAGAACGAGCCGTTTCTTGATTGAACCGCTCGGCGATTGAGAGTTTCTAGTGGGCCATTTTTGGTAAGCAGAACTGGCGATGCGGGATGAACCGAACGCGAGGTTAAGGTGCCGGAATTCACGCTCATCAGACACCACAAAAGGTGTTAGTTCATCTAGACAGCAGGACGGTGGCCATGGAAGTCGGAATCCGCTAAGGAGTGTGTAACAACTCACCTGCCGAATGAACTAGCCCTGAAAATGGATGGCGCTTAAGCGTGATACCCATACCTCGCCGTCAGCGTTGAAGTGACGCGCTGACGAGTAGGCAGGCGTGGAGGTCAGTGAAGAAGCCTTGGCAGTGATGCTGGGTGAAACGGCCTCC";
		EdlibAlignResult result = edlibAlign(query, static_cast<int>(std::strlen(query)),
			target, static_cast<int>(std::strlen(target)),
			edlibNewAlignConfig(-1, EDLIB_MODE_HW, EDLIB_TASK_LOC, additionalEqualities, 24));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 0));
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		edlibFreeAlignResult(result);
	}
	{ // bool test13() 
		// In this test, one of optimal solutions is:
		//         B
		//       AA
		// which brings us into interesting situation where one of end locations is -1.
		const char* query = "AA";
		const char* target = "B";
		EdlibAlignResult result = edlibAlign(query, static_cast<int>(std::strlen(query)),
			target, static_cast<int>(std::strlen(target)),
			edlibNewAlignConfig(-1, EDLIB_MODE_HW, EDLIB_TASK_PATH, NULL, 0));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 2));
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		edlibFreeAlignResult(result);
	}
	{ // 
		// In this test, one of optimal solutions is:
		//         B
		//       AA
		// which brings us into interesting situation where one of end locations is -1.
		const char* query = "AA";
		const char* target = "B";
		EdlibAlignResult result = edlibAlign(query, static_cast<int>(std::strlen(query)),
			target, static_cast<int>(std::strlen(target)),
			edlibNewAlignConfig(-1, EDLIB_MODE_SHW, EDLIB_TASK_PATH, NULL, 0));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 2));
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		edlibFreeAlignResult(result);
	}
	{ // bool test15() 
		// In this test, optimal alignment is when query and target overlap, query end with target start, HW.
		const char* query = "AAABBB";
		const char* target = "BBBC";
		EdlibAlignResult result = edlibAlign(query, static_cast<int>(std::strlen(query)),
			target, static_cast<int>(std::strlen(target)),
			edlibNewAlignConfig(-1, EDLIB_MODE_HW, EDLIB_TASK_LOC, NULL, 0));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 3));
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		edlibFreeAlignResult(result);
	}
	{ // bool test16() 
		// In this test, optimal alignment is when query and target overlap, query start with target end, HW.
		const char* query = "BBBAAA";
		const char* target = "CBBB";
		EdlibAlignResult result = edlibAlign(query, static_cast<int>(std::strlen(query)),
			target, static_cast<int>(std::strlen(target)), edlibNewAlignConfig(-1, EDLIB_MODE_HW, EDLIB_TASK_LOC, NULL, 0));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 3));
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		edlibFreeAlignResult(result);
	}
	{ // bool testCigar() 
		const uchar alignment[] = {
			EDLIB_EDOP_MATCH, EDLIB_EDOP_MATCH, EDLIB_EDOP_INSERT, EDLIB_EDOP_INSERT,
			EDLIB_EDOP_INSERT, EDLIB_EDOP_DELETE, EDLIB_EDOP_INSERT, EDLIB_EDOP_INSERT,
			EDLIB_EDOP_MISMATCH, EDLIB_EDOP_MATCH, EDLIB_EDOP_MATCH
		};
		char * cigar = edlibAlignmentToCigar(alignment, 11, EDLIB_CIGAR_EXTENDED);
		bool pass = true;
		char expected[] = "2=3I1D2I1X2=";
		if(strcmp(cigar, expected) != 0) {
			pass = false;
			printf("Expected %s, got %s\n", expected, cigar);
		}
		printf("Cigar extended: ");
		printf(pass ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		SAlloc::F(cigar);
		cigar = edlibAlignmentToCigar(alignment, 11, EDLIB_CIGAR_STANDARD);
		pass = true;
		char expected2[] = "2M3I1D2I3M";
		SLCHECK_Z(strcmp(cigar, expected2));
			//pass = false;
			//printf("Expected %s, got %s\n", expected2, cigar);
		printf("Cigar standard: ");
		printf(pass ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		SAlloc::F(cigar);
	}
	{ // bool testCustomEqualityRelation() 
		const EdlibEqualityPair additionalEqualities[6] = {{'R', 'A'}, {'R', 'G'}, {'N', 'A'}, {'N', 'C'}, {'N', 'T'}, {'N', 'G'}};
		//bool allPass = true;
		const char * query =  "GTGNRTCARCGAANCTTTN";
		const char * target = "GTGAGTCATCGAATCTTTGAACGCACCTTGCGCTCCTTGGT";
		printf("Degenerate nucleotides (HW): ");
		EdlibAlignResult result = edlibAlign(query, 19, target, 41,
			edlibNewAlignConfig(-1, EDLIB_MODE_HW, EDLIB_TASK_PATH, additionalEqualities, 6));
		SLCHECK_NZ(local_ok = (result.status == EDLIB_STATUS_OK && result.editDistance == 1));
		edlibFreeAlignResult(result);
		printf(local_ok ? "\x1B[32m" "OK" "\x1B[0m\n" : "\x1B[31m" "FAIL" "\x1B[0m\n");
		//allPass = allPass && pass;
	}
	{ // bool testEmptySequences() 
		printf("Empty query or target:\n");
		const char * emptySeq =  "";
		const char * nonEmptySeq = "ACTG";
		const int nonEmptySeqLength = 4;
		bool r = true;
		SLCHECK_NZ(executeTest(emptySeq, 0, nonEmptySeq, nonEmptySeqLength, EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(nonEmptySeq, nonEmptySeqLength, emptySeq, 0, EDLIB_MODE_NW));
		SLCHECK_NZ(executeTest(emptySeq, 0, nonEmptySeq, nonEmptySeqLength, EDLIB_MODE_SHW));
		SLCHECK_NZ(executeTest(nonEmptySeq, nonEmptySeqLength, emptySeq, 0, EDLIB_MODE_SHW));
		SLCHECK_NZ(executeTest(emptySeq, 0, nonEmptySeq, nonEmptySeqLength, EDLIB_MODE_HW));
		SLCHECK_NZ(executeTest(nonEmptySeq, nonEmptySeqLength, emptySeq, 0, EDLIB_MODE_HW));
	}
	{
		// This program has optional first parameter, which is number of random tests to run
		// per each algorithm. Default is 100.
		int numRandomTests = 100;
		srand(42);
		bool allTestsPassed = true;
		printf("Testing HW with alignment...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_HW, true));
		printf("\n");

		printf("Testing HW...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_HW, false));
		printf("\n");

		printf("Testing NW with alignment...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_NW, true));
		printf("\n");

		printf("Testing NW...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_NW, false));
		printf("\n");

		printf("Testing SHW with alignment...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_SHW, true));
		printf("\n");

		printf("Testing SHW...\n");
		SLCHECK_NZ(RunRandomTests(numRandomTests, EDLIB_MODE_SHW, false));
		printf("\n");
	}
	{
		SString temp_buf;
		SString in_file_path;
		SString out_file_path;
		SString pattern_buf;
		SString line_buf;
		StrAssocArray str_list;
		LAssocArray dist_list;
		{
			const SIntToSymbTabEntry best_results[] = {
				3, "В здоровом теле здоровый друг.",
				3, "В здоровом теле здоровый пук."
			};
			SLS.QueryPath("testroot", in_file_path);
			in_file_path.SetLastSlash().Cat("data").SetLastSlash().Cat("phrases-ru-1251.txt");
			SLS.QueryPath("testroot", out_file_path);
			out_file_path.SetLastSlash().Cat("out").SetLastSlash().Cat("phrases-ru-1251-distance.txt");
			SFile f_in(in_file_path, SFile::mRead);	
			(pattern_buf = "В здоровом теле здоровый дух").Transf(CTRANSF_UTF8_TO_OUTER);
			if(f_in.IsValid()) {
				str_list.Z();
				dist_list.clear();
				long line_no = 0;
				while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
					line_no++;
					EdlibAlignResult result = edlibAlign(pattern_buf, pattern_buf.Len(), 
						line_buf, line_buf.Len(), edlibNewAlignConfig(-1, EDLIB_MODE_NW, EDLIB_TASK_DISTANCE, 0, 0));				
					if(result.editDistance >= 0) {
						str_list.AddFast(line_no, line_buf);
						dist_list.Add(line_no, result.editDistance);
					}
				}
				{
					dist_list.SortByVal();
					{
						SLCHECK_NZ(dist_list.getCount() >= SIZEOFARRAY(best_results));
						for(uint i = 0; i < SIZEOFARRAY(best_results); i++) {
							bool found = false;
							for(uint j = 0; !found && j < SIZEOFARRAY(best_results); j++) {
								const LAssoc & r_dist = dist_list.at(j);
								const long line_idx = r_dist.Key;
								const long distance = r_dist.Val;
								str_list.GetText(line_idx, temp_buf);
								temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
								if(temp_buf.IsEqiUtf8(best_results[i].P_Symb)) {
									SLCHECK_EQ(distance, (long)best_results[i].Id);
									found = true;
								}
							}
							SLCHECK_NZ(found);
						}
					}
					SFile f_out(out_file_path, SFile::mWrite);
					if(f_out.IsValid()) {
						f_out.WriteLine(line_buf.Z().Cat("pattern").CatDiv(':', 2).Cat(pattern_buf).CR());
						f_out.WriteBlancLine();
						for(uint i = 0; i < dist_list.getCount(); i++) {
							const LAssoc & r_dist = dist_list.at(i);
							const long line_idx = r_dist.Key;
							const long distance = r_dist.Val;
							str_list.GetText(line_idx, temp_buf);
							f_out.WriteLine(line_buf.Z().Cat(distance).Tab().Cat(temp_buf).CR());
						}
					}
				}
			}
		}
	}
	return CurrentStatus;
}
