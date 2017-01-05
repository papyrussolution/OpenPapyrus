/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*             CLIPS Version 6.24  06/05/06            */
/*                                                     */
/*                 SYMBOL HEADER FILE                  */
/*******************************************************/

/*************************************************************/
/* Purpose: Manages the atomic data value hash tables for    */
/*   storing symbols, integers, floats, and bit maps.        */
/*   Contains routines for adding entries, examining the     */
/*   hash tables, and performing garbage collection to       */
/*   remove entries no longer in use.                        */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Support for run-time programs directly passing */
/*            the hash tables for initialization.            */
/*                                                           */
/*************************************************************/

#ifndef _H_symbol
#define _H_symbol

#ifdef LOCALE
 #undef LOCALE
#endif

#ifdef _SYMBOL_SOURCE_
 #define LOCALE
#else
 #define LOCALE extern
#endif

#ifndef SYMBOL_HASH_SIZE
 #define SYMBOL_HASH_SIZE       63559L
#endif

#ifndef FLOAT_HASH_SIZE
 #define FLOAT_HASH_SIZE         8191
#endif

#ifndef INTEGER_HASH_SIZE
 #define INTEGER_HASH_SIZE       8191
#endif

#ifndef BITMAP_HASH_SIZE
 #define BITMAP_HASH_SIZE        8191
#endif

/************************************************************/
/* symbolHashNode STRUCTURE:                                */
/************************************************************/
struct symbolHashNode {
	struct symbolHashNode * next;
	long count;
	int depth;
	unsigned int permanent : 1;
	unsigned int markedEphemeral : 1;
	unsigned int neededSymbol : 1;
	unsigned int bucket : 29;
	char * contents;
};

/************************************************************/
/* floatHashNode STRUCTURE:                                  */
/************************************************************/
struct floatHashNode {
	struct floatHashNode * next;
	long count;
	int depth;
	unsigned int permanent : 1;
	unsigned int markedEphemeral : 1;
	unsigned int neededFloat : 1;
	unsigned int bucket : 29;
	double contents;
};

/************************************************************/
/* integerHashNode STRUCTURE:                               */
/************************************************************/
struct integerHashNode {
	struct integerHashNode * next;
	long count;
	int depth;
	unsigned int permanent : 1;
	unsigned int markedEphemeral : 1;
	unsigned int neededInteger : 1;
	unsigned int bucket : 29;
	long int contents;
};

/************************************************************/
/* bitMapHashNode STRUCTURE:                                */
/************************************************************/
struct bitMapHashNode {
	struct bitMapHashNode * next;
	long count;
	int depth;
	unsigned int permanent : 1;
	unsigned int markedEphemeral : 1;
	unsigned int neededBitMap : 1;
	unsigned int bucket : 29;
	char * contents;
	unsigned short size;
};

/************************************************************/
/* genericHashNode STRUCTURE:                               */
/************************************************************/
struct genericHashNode {
	struct genericHashNode * next;
	long count;
	int depth;
	unsigned int permanent : 1;
	unsigned int markedEphemeral : 1;
	unsigned int needed : 1;
	unsigned int bucket : 29;
};

typedef struct symbolHashNode SYMBOL_HN;
typedef struct floatHashNode FLOAT_HN;
typedef struct integerHashNode INTEGER_HN;
typedef struct bitMapHashNode BITMAP_HN;
typedef struct genericHashNode GENERIC_HN;

/**********************************************************/
/* EPHEMERON STRUCTURE: Data structure used to keep track */
/*   of ephemeral symbols, floats, and integers.          */
/*                                                        */
/*   associatedValue: Contains a pointer to the storage   */
/*   structure for the symbol, float, or integer which is */
/*   ephemeral.                                           */
/*                                                        */
/*   next: Contains a pointer to the next ephemeral item  */
/*   in a list of ephemeral items.                        */
/**********************************************************/
struct ephemeron {
	GENERIC_HN * associatedValue;
	struct ephemeron * next;
};

/************************************************************/
/* symbolMatch STRUCTURE:                               */
/************************************************************/
struct symbolMatch {
	struct symbolHashNode * match;
	struct symbolMatch * next;
};

#define ValueToString(target) (((struct symbolHashNode *)(target))->contents)
#define ValueToDouble(target) (((struct floatHashNode *)(target))->contents)
#define ValueToLong(target) (((struct integerHashNode *)(target))->contents)
#define ValueToInteger(target) ((int)(((struct integerHashNode *)(target))->contents))
#define ValueToBitMap(target) ((void *)((struct bitMapHashNode *)(target))->contents)

#define EnvValueToString(theEnv, target) (((struct symbolHashNode *)(target))->contents)
#define EnvValueToDouble(theEnv, target) (((struct floatHashNode *)(target))->contents)
#define EnvValueToLong(theEnv, target) (((struct integerHashNode *)(target))->contents)
#define EnvValueToInteger(theEnv, target) ((int)(((struct integerHashNode *)(target))->contents))
#define EnvValueToBitMap(theEnv, target) ((void *)((struct bitMapHashNode *)(target))->contents)

#define IncrementSymbolCount(theValue) (((SYMBOL_HN *)theValue)->count++)
#define IncrementFloatCount(theValue) (((FLOAT_HN *)theValue)->count++)
#define IncrementIntegerCount(theValue) (((INTEGER_HN *)theValue)->count++)
#define IncrementBitMapCount(theValue) (((BITMAP_HN *)theValue)->count++)

/*==================*/
/* ENVIRONMENT DATA */
/*==================*/

#define SYMBOL_DATA 49

struct symbolData {
	void * TrueSymbolHN;
	void * FalseSymbolHN;
	void * PositiveInfinity;
	void * NegativeInfinity;
	void * Zero;
	SYMBOL_HN ** SymbolTable;
	FLOAT_HN ** FloatTable;
	INTEGER_HN ** IntegerTable;
	BITMAP_HN ** BitMapTable;
	struct ephemeron * EphemeralSymbolList;
	struct ephemeron * EphemeralFloatList;
	struct ephemeron * EphemeralIntegerList;
	struct ephemeron * EphemeralBitMapList;
#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE || BLOAD_INSTANCES || BSAVE_INSTANCES
	long NumberOfSymbols;
	long NumberOfFloats;
	long NumberOfIntegers;
	long NumberOfBitMaps;
	SYMBOL_HN ** SymbolArray;
	struct floatHashNode ** FloatArray;
	INTEGER_HN ** IntegerArray;
	BITMAP_HN ** BitMapArray;
#endif
};

#define SymbolData(theEnv) ((struct symbolData *)GetEnvironmentData(theEnv, SYMBOL_DATA))
#define EnvFalseSymbol(theEnv) SymbolData(theEnv)->FalseSymbolHN
#define EnvTrueSymbol(theEnv) SymbolData(theEnv)->TrueSymbolHN

#if ENVIRONMENT_API_ONLY
 #define FalseSymbol(theEnv) SymbolData(theEnv)->FalseSymbolHN
 #define TrueSymbol(theEnv) SymbolData(theEnv)->TrueSymbolHN
 #define AddSymbol(theEnv, a) EnvAddSymbol(theEnv, a)
 #define AddLong(theEnv, a) EnvAddLong(theEnv, a)
 #define AddDouble(theEnv, a) EnvAddDouble(theEnv, a)
#else
 #define FalseSymbol() SymbolData(GetCurrentEnvironment())->FalseSymbolHN
 #define TrueSymbol() SymbolData(GetCurrentEnvironment())->TrueSymbolHN
 #define AddSymbol(a) EnvAddSymbol(GetCurrentEnvironment(), a)
 #define AddLong(a) EnvAddLong(GetCurrentEnvironment(), a)
 #define AddDouble(a) EnvAddDouble(GetCurrentEnvironment(), a)
#endif

LOCALE void InitializeAtomTables(void *, struct symbolHashNode **, struct floatHashNode **, struct integerHashNode **, struct bitMapHashNode **);
LOCALE void * EnvAddSymbol(void *, char *);
LOCALE SYMBOL_HN * FindSymbolHN(void *, char *);
LOCALE void * EnvAddDouble(void *, double);
LOCALE void * EnvAddLong(void *, long int);
LOCALE void * AddBitMap(void *, void *, unsigned);
LOCALE INTEGER_HN * FindLongHN(void *, long int);
LOCALE unsigned long HashSymbol(char *, unsigned long);
LOCALE unsigned HashFloat(double, unsigned);
LOCALE unsigned HashInteger(long int, unsigned);
LOCALE unsigned HashBitMap(char *, unsigned, unsigned);
LOCALE void DecrementSymbolCount(void *, struct symbolHashNode *);
LOCALE void DecrementFloatCount(void *, struct floatHashNode *);
LOCALE void DecrementIntegerCount(void *, struct integerHashNode *);
LOCALE void DecrementBitMapCount(void *, struct bitMapHashNode *);
LOCALE void RemoveEphemeralAtoms(void *);
LOCALE struct symbolHashNode ** GetSymbolTable(void *);
LOCALE void SetSymbolTable(void *, struct symbolHashNode **);
LOCALE struct floatHashNode ** GetFloatTable(void *);
LOCALE void SetFloatTable(void *, struct floatHashNode **);
LOCALE struct integerHashNode ** GetIntegerTable(void *);
LOCALE void SetIntegerTable(void *, struct integerHashNode **);
LOCALE struct bitMapHashNode ** GetBitMapTable(void *);
LOCALE void SetBitMapTable(void *, struct bitMapHashNode **);
LOCALE void RefreshSpecialSymbols(void *);
LOCALE struct symbolMatch * FindSymbolMatches(void *, char *, unsigned *, unsigned *);
LOCALE void ReturnSymbolMatches(void *, struct symbolMatch *);
LOCALE SYMBOL_HN * GetNextSymbolMatch(void *, char *, unsigned, SYMBOL_HN *, int, unsigned *);
LOCALE void ClearBitString(void *, unsigned);
LOCALE void SetAtomicValueIndices(void *, int);
LOCALE void RestoreAtomicValueBuckets(void *);

#endif

