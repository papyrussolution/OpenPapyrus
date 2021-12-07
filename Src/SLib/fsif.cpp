/*
MODULEINF("1992-08-01", "1990-1992 Erik Bachmann (E-mail: ebp@dde.dk" );
+++Date last modified: 05-Jul-1997

Modified by Anton Sobolev: Jan-2001, 2010, 2016
*/
#include <slib-internal.h>
#pragma hdrstop
/*
	FIND_STR_IN_FILE
	Searches a binary file for a string.
	Returns the start offset for the first occurrence of the string.

	CALL:
		find_str_in_file(Filename, startoffset, string);
	ARGUMENTS:
		fpFile      : Pointer to the (open) binary file
		lOffset     : Startoffset for search
		pszStr      : String to search for
	PROTOTYPE:
		long _CfnTYPE find_str_in_file(FILE *fpFile, long lOffset, char *pszStr);
	RETURN VALUE:
		long lStatusFlag   :
			-1   : String not found
			-2   : Not enough memory to perform search
			otherwise : Offset for string in file
	MODULE:
		fsif.c

1994-03-18/Bac
	-  Enhanced error detection. Returning error value
1995-06-19/Bac
	-  Patch for returning position. Changed from blockwise to bytewise calculation.
1995-09-20/Bac
	-  Ajusted secondary search in last block
1995-10-27/Bac
	-  Released to public domain

1992-08-01/Erik Bachmann
*/

#define BUFFERSIZE  512

long SearchStrInFile(FILE * fpFile, long lOffset, const char *pszStr, int ignoreCase)
{
	long lStatusFlag = -1l; /* Status: -1 = not found */
	long lPosInFile  = 0l; /* Position in file       */
	long lCurrentPos = 0l;
	char * pszBuffer = NULL; /* Buffer for fileinput        */
	int  cFound      = 0;
	size_t data_size = 0; /* Size of data read from file */
	int  iSector     = 0; /* No of blocks read           */
	int  iBufferSize = BUFFERSIZE;
	int  iNoBlocks   = 0; /* No of blocks remaining      */
	int  cmp_result  = 0;
	size_t offset      = 0; /* local counter               */
	size_t pattern_len = strlen(pszStr);

	lPosInFile = filelength(fileno(fpFile)); // Find filesize
	// @v10.3.6 (see below) iNoBlocks = (int)(lPosInFile / (int) iBufferSize) ;
	// Calculate remaining no of blocks 
	fseek(fpFile, lOffset, SEEK_SET); // Go to start offset 
	lCurrentPos = lOffset;
	iNoBlocks = (lPosInFile - lOffset) / iBufferSize;
	// Calculate remaining no of blocks 
	if((pszBuffer = static_cast<char *>(SAlloc::C(2 * iBufferSize, sizeof(char)))) == 0)
		return -2;
	memzero(pszBuffer, iBufferSize);
	data_size = fread(pszBuffer, sizeof(char), iBufferSize, fpFile);
	// Read the first block 
	while(iNoBlocks > 0 && !cFound) { /* Repeat until EOF or found */
		iSector++; /* Counting no of blocks read */
		iNoBlocks--;
		memzero(&pszBuffer[iBufferSize], iBufferSize);
		// Read next block 
		data_size = fread(&pszBuffer[iBufferSize], sizeof(char), iBufferSize, fpFile);
		// Search first block 
		for(offset = 0; offset < (size_t)iBufferSize; offset++) {
			if(ignoreCase)
				cmp_result = strnicmp866(pszBuffer+offset, pszStr, pattern_len);
			else
				cmp_result = strncmp(pszBuffer+offset, pszStr, pattern_len);
			if(cmp_result == 0) {
				// Is the string placed here?
				cFound = 1; // Yes -> set flag
				break;
			}
			else
				lCurrentPos++; // No -> Try again
		}
		memcpy(pszBuffer, &pszBuffer[iBufferSize], iBufferSize); // Shift block left
	}
	// Search the last Sector read if tag not found yet
	if(!cFound) {
		iSector++; // Counting no of blocks read 
		for(offset = 0; ((int)offset) < ((int)(data_size - pattern_len)); offset++) {
			// Is the string placed in last block?
			if(ignoreCase)
				cmp_result = strnicmp866(pszBuffer+offset, pszStr, pattern_len);
			else
				cmp_result = strncmp(pszBuffer+offset, pszStr, pattern_len);
			if(cmp_result == 0) {
				cFound = 1;
				break;
			}
			else if(lCurrentPos < lPosInFile) // Check for End of File
				lCurrentPos++; /* In file -> goto next */
			else
				break;
		}
	}
	SAlloc::F(pszBuffer);
	return cFound ? ((long)lCurrentPos) : lStatusFlag;
}

#if 0 /* { */

int main(int argc, char * argv[])
{
	long pos = 0L;
	FILE * f = NULL;
	if(argc < 3) {
		printf("FSIF\nUsage: fsif filename search_pattern\n");
		return -1;
	}
	f = fopen(argv[1], "rb");
	if(f == 0) {
		printf("Error opening file %s\n", argv[1]);
		return -1;
	}
	pos = SearchStrInFile(f, 0L, argv[2]);
	if(pos > 0) {
		printf("String is founded (offset = %ld)\n", pos);
		return 0;
	}
	else {
		if(pos == -1)
			printf("String is not founded\n");
		else
			printf("Error\n");
		return -1;
	}
}

#endif /* 0 } */
