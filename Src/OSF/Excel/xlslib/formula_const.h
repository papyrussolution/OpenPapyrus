/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010-2013 Ger Hobbelt All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef FORMULA_CONST_H
#define FORMULA_CONST_H

namespace xlslib_core
{
	static const uint32 invalidIndex = 0xFFFFFFFF;

	typedef enum cell_addr_mode_t
	{
		CELL_RELATIVE_A1   = 0xC000,
		CELL_ABSOLUTE_As1  = 0x8000,
		CELL_ABSOLUTE_sA1  = 0x4000,
		CELL_ABSOLUTE_sAs1 = 0,
	} cell_addr_mode_t;

	// 'operand class'
	typedef enum cell_op_class_t
	{
		CELLOP_AS_VALUE     = 0x40, //  V  - value, i.e. the value stored in the cell
		CELLOP_AS_REFERENCE = 0x20, // [R] - reference, i.e. the cell address itself
		CELLOP_AS_ARRAY     = 0x60, //  A  - array, i.e. the cell address in {...} array form
	} cell_op_class_t;

	enum expr_operator_code_t
	{
		OP_EXP = 0x01,                                 // ptgExp          01h   control
		OP_TBL = 0x02,                                 // ptgTbl          02h   control
		OP_ADD = 0x03,                                 // ptgAdd          03h   operator
		OP_SUB = 0x04,                                 // ptgSub          04h   operator
		OP_MUL = 0x05,                                 // ptgMul          05h   operator
		OP_DIV = 0x06,                                 // ptgDiv          06h   operator
		OP_POWER = 0x07,                               // ptgPower        07h   operator
		OP_CONCAT = 0x08,                              // ptgConcat       08h   operator
		OP_LT = 0x09,                                  // ptgLT           09h   operator
		OP_LE = 0x0A,                                  // ptgLE           0Ah   operator
		OP_EQ = 0x0B,                                  // ptgEQ           0Bh   operator
		OP_GE = 0x0C,                                  // ptgGE           0Ch   operator
		OP_GT = 0x0D,                                  // ptgGT           0Dh   operator
		OP_NE = 0x0E,                                  // ptgNE           0Eh   operator
		OP_ISECT = 0x0F,                               // ptgIsect        0Fh   operator
		OP_UNION = 0x10,                               // ptgUnion        10h   operator
		OP_RANGE = 0x11,                               // ptgRange        11h   operator
		OP_UPLUS = 0x12,                               // ptgUplus        12h   operator
		OP_UMINUS = 0x13,                              // ptgUminus       13h   operator
		OP_PERCENT = 0x14,                             // ptgPercent      14h   operator
		OP_PAREN = 0x15,                               // ptgParen        15h   control
		OP_MISSARG = 0x16,                             // ptgMissArg      16h   operand
		OP_STR = 0x17,                                 // ptgStr          17h   operand
		OP_ATTR = 0x19,                                // ptgAttr         19h   control
		OP_SHEET = 0x1A,                               // ptgSheet        1Ah   (ptg DELETED)
		OP_ENDSHEET = 0x1B,                            // ptgEndSheet     1Bh   (ptg DELETED)
		OP_ERR = 0x1C,                                 // ptgErr          1Ch   operand
		OP_BOOL = 0x1D,                                // ptgBool         1Dh   operand
		OP_INT = 0x1E,                                 // ptgInt          1Eh   operand
		OP_NUM = 0x1F,                                 // ptgNum          1Fh   operand
		OP_ARRAY = 0x20,                               // ptgArray        20h   operand, reference class
		OP_FUNC = 0x21,                                // ptgFunc         21h   operator
		OP_FUNCVAR = 0x22,                             // ptgFuncVar      22h   operator
		OP_NAME = 0x23,                                // ptgName         23h   operand, reference class
		OP_REF = 0x24,                                 // ptgRef          24h   operand, reference class
		OP_AREA = 0x25,                                // ptgArea         25h   operand, reference class
		OP_MEMAREA = 0x26,                             // ptgMemArea      26h   operand, reference class
		OP_MEMERR = 0x27,                              // ptgMemErr       27h   operand, reference class
		OP_MEMNOMEM = 0x28,                            // ptgMemNoMem     28h   control
		OP_MEMFUNC = 0x29,                             // ptgMemFunc      29h   control
		OP_REFERR = 0x2A,                              // ptgRefErr       2Ah   operand, reference class
		OP_AREAERR = 0x2B,                             // ptgAreaErr      2Bh   operand, reference class
		OP_REFN = 0x2C,                                // ptgRefN         2Ch   operand, reference class
		OP_AREAN = 0x2D,                               // ptgAreaN        2Dh   operand, reference class
		OP_MEMAREAN = 0x2E,                            // ptgMemAreaN     2Eh   control
		OP_MEMNOMEMN = 0x2F,                           // ptgMemNoMemN    2Fh   control
		OP_NAMEX = 0x39,                               // ptgNameX        39h   operand, reference class
		OP_REF3D = 0x3A,                               // ptgRef3d        3Ah   operand, reference class
		OP_AREA3D = 0x3B,                              // ptgArea3d       3Bh   operand, reference class
		OP_REFERR3D = 0x3C,                            // ptgRefErr3d     3Ch   operand, reference class
		OP_AREAERR3D = 0x3D,                           // ptgAreaErr3d    3Dh   operand, reference class
		OP_ARRAYV = 0x40,                              // ptgArrayV       40h   operand, value class
		OP_FUNCV = 0x41,                               // ptgFuncV        41h   operator
		OP_FUNCVARV = 0x42,                            // ptgFuncVarV     42h   operator
		OP_NAMEV = 0x43,                               // ptgNameV        43h   operand, value class
		OP_REFV = 0x44,                                // ptgRefV         44h   operand, value class
		OP_AREAV = 0x45,                               // ptgAreaV        45h   operand, value class
		OP_MEMAREAV = 0x46,                            // ptgMemAreaV     46h   operand, value class
		OP_MEMERRV = 0x47,                             // ptgMemErrV      47h   operand, value class
		OP_MEMNOMEMV = 0x48,                           // ptgMemNoMemV    48h   control
		OP_MEMFUNCV = 0x49,                            // ptgMemFuncV     49h   control
		OP_REFERRV = 0x4A,                             // ptgRefErrV      4Ah   operand, value class
		OP_AREAERRV = 0x4B,                            // ptgAreaErrV     4Bh   operand, value class
		OP_REFNV = 0x4C,                               // ptgRefNV        4Ch   operand, value class
		OP_AREANV = 0x4D,                              // ptgAreaNV       4Dh   operand, value class
		OP_MEMAREANV = 0x4E,                           // ptgMemAreaNV    4Eh   control
		OP_MEMNOMEMNV = 0x4F,                          // ptgMemNoMemNV   4Fh   control
		OP_FUNCCEV = 0x58,                             // ptgFuncCEV      58h   operator
		OP_NAMEXV = 0x59,                              // ptgNameXV       59h   operand, value class
		OP_REF3DV = 0x5A,                              // ptgRef3dV       5Ah   operand, value class
		OP_AREA3DV = 0x5B,                             // ptgArea3dV      5Bh   operand, value class
		OP_REFERR3DV = 0x5C,                           // ptgRefErr3dV    5Ch   operand, value class
		OP_AREAERR3DV = 0x5D,                          // ptgAreaErr3dV   5Dh   operand, value class
		OP_ARRAYA = 0x60,                              // ptgArrayA       60h   operand, array class
		OP_FUNCA = 0x61,                               // ptgFuncA        61h   operator
		OP_FUNCVARA = 0x62,                            // ptgFuncVarA     62h   operator
		OP_NAMEA = 0x63,                               // ptgNameA        63h   operand, array class
		OP_REFA = 0x64,                                // ptgRefA         64h   operand, array class
		OP_AREAA = 0x65,                               // ptgAreaA        65h   operand, array class
		OP_MEMAREAA = 0x66,                            // ptgMemAreaA     66h   operand, array class
		OP_MEMERRA = 0x67,                             // ptgMemErrA      67h   operand, array class
		OP_MEMNOMEMA = 0x68,                           // ptgMemNoMemA    68h   control
		OP_MEMFUNCA = 0x69,                            // ptgMemFuncA     69h   control
		OP_REFERRA = 0x6A,                             // ptgRefErrA      6Ah   operand, array class
		OP_AREAERRA = 0x6B,                            // ptgAreaErrA     6Bh   operand, array class
		OP_REFNA = 0x6C,                               // ptgRefNA        6Ch   operand, array class
		OP_AREANA = 0x6D,                              // ptgAreaNA       6Dh   operand, array class
		OP_MEMAREANA = 0x6E,                           // ptgMemAreaNA    6Eh   control
		OP_MEMNOMEMNA = 0x6F,                          // ptgMemNoMemNA   6Fh   control
		OP_FUNCCEA = 0x78,                             // ptgFuncCEA      78h   operator
		OP_NAMEXA = 0x79,                              // ptgNameXA       79h   operand, array class (NEW ptg)
		OP_REF3DA = 0x7A,                              // ptgRef3dA       7Ah   operand, array class (NEW ptg)
		OP_AREA3DA = 0x7B,                             // ptgArea3dA      7Bh   operand, array class (NEW ptg)
		OP_REFERR3DA = 0x7C,                           // ptgRefErr3dA    7Ch   operand, array class (NEW ptg)
		OP_AREAERR3DA = 0x7D,                          // ptgAreaErr3dA   7Dh   operand, array class (NEW ptg)
	};

	enum
	{
		A_UNKNOWN = 0U,
		A_0 = 0x0001,
		A_1 = 0x0002,
		A_2 = 0x0004,
		A_3 = 0x0008,
		A_4 = 0x0010,
		A_5 = 0x0020,
		A_6 = 0x0040,
		A_0_OR_1 = 0x0003,
		A_0_TO_2 = 0x0007,
		A_0_TO_3 = 0x000F,
		A_0_TO_4 = 0x001F,
		A_0_TO_5 = 0x003F,
		A_1_OR_2 = 0x0006,
		A_1_OR_MORE = 0x7FFE,
		A_2_OR_MORE = 0x7FFC,
		A_1_TO_3 = 0x000E,
		A_1_TO_4 = 0x001E,
		A_1_TO_5 = 0x003E,
		A_1_TO_7 = 0x00FE,
		A_2_OR_3 = 0x000C,
		A_2_TO_4 = 0x001C,
		A_2_TO_5 = 0x003C,
		A_2_TO_9 = 0x03FC,
		A_3_OR_MORE = 0x7FF8,
		A_3_OR_4 = 0x0018,
		A_3_TO_5 = 0x0038,
		A_3_TO_6 = 0x0078,
		A_4_OR_5 = 0x0030,
		A_4_TO_6 = 0x0070,
		A_5_OR_6 = 0x0060,
		A_5_TO_7 = 0x00E0,
		A_6_OR_7 = 0x00C0,
		A_6_TO_8 = 0x01C0,
		A_7_OR_8 = 0x0180,
		A_8_OR_9 = 0x0300,

		A_MACRO = 0x8000,
	};

#if !defined(xlUDF)
// XLCALL.H not loaded

	/*
	 * User defined function
	 *
	 * First argument should be a function reference.
	 */
#define xlUDF      255

	/*
	 * Built-in Excel functions and command equivalents
	 */
#define xlfCount 0
#define xlfIf 1
#define xlfIsna 2
#define xlfIserror 3
#define xlfSum 4
#define xlfAverage 5
#define xlfMin 6
#define xlfMax 7
#define xlfRow 8
#define xlfColumn 9
#define xlfNa 10
#define xlfNpv 11
#define xlfStdev 12
#define xlfDollar 13
#define xlfFixed 14
#define xlfSin 15
#define xlfCos 16
#define xlfTan 17
#define xlfAtan 18
#define xlfPi 19
#define xlfSqrt 20
#define xlfExp 21
#define xlfLn 22
#define xlfLog10 23
#define xlfAbs 24
#define xlfInt 25
#define xlfSign 26
#define xlfRound 27
#define xlfLookup 28
#define xlfIndex 29
#define xlfRept 30
#define xlfMid 31
#define xlfLen 32
#define xlfValue 33
#define xlfTrue 34
#define xlfFalse 35
#define xlfAnd 36
#define xlfOr 37
#define xlfNot 38
#define xlfMod 39
#define xlfDcount 40
#define xlfDsum 41
#define xlfDaverage 42
#define xlfDmin 43
#define xlfDmax 44
#define xlfDstdev 45
#define xlfVar 46
#define xlfDvar 47
#define xlfText 48
#define xlfLinest 49
#define xlfTrend 50
#define xlfLogest 51
#define xlfGrowth 52
#define xlfGoto 53
#define xlfHalt 54
#define xlfPv 56
#define xlfFv 57
#define xlfNper 58
#define xlfPmt 59
#define xlfRate 60
#define xlfMirr 61
#define xlfIrr 62
#define xlfRand 63
#define xlfMatch 64
#define xlfDate 65
#define xlfTime 66
#define xlfDay 67
#define xlfMonth 68
#define xlfYear 69
#define xlfWeekday 70
#define xlfHour 71
#define xlfMinute 72
#define xlfSecond 73
#define xlfNow 74
#define xlfAreas 75
#define xlfRows 76
#define xlfColumns 77
#define xlfOffset 78
#define xlfAbsref 79
#define xlfRelref 80
#define xlfArgument 81
#define xlfSearch 82
#define xlfTranspose 83
#define xlfError 84
#define xlfStep 85
#define xlfType 86
#define xlfEcho 87
#define xlfSetName 88
#define xlfCaller 89
#define xlfDeref 90
#define xlfWindows 91
#define xlfSeries 92
#define xlfDocuments 93
#define xlfActiveCell 94
#define xlfSelection 95
#define xlfResult 96
#define xlfAtan2 97
#define xlfAsin 98
#define xlfAcos 99
#define xlfChoose 100
#define xlfHlookup 101
#define xlfVlookup 102
#define xlfLinks 103
#define xlfInput 104
#define xlfIsref 105
#define xlfGetFormula 106
#define xlfGetName 107
#define xlfSetValue 108
#define xlfLog 109
#define xlfExec 110
#define xlfChar 111
#define xlfLower 112
#define xlfUpper 113
#define xlfProper 114
#define xlfLeft 115
#define xlfRight 116
#define xlfExact 117
#define xlfTrim 118
#define xlfReplace 119
#define xlfSubstitute 120
#define xlfCode 121
#define xlfNames 122
#define xlfDirectory 123
#define xlfFind 124
#define xlfCell 125
#define xlfIserr 126
#define xlfIstext 127
#define xlfIsnumber 128
#define xlfIsblank 129
#define xlfT 130
#define xlfN 131
#define xlfFopen 132
#define xlfFclose 133
#define xlfFsize 134
#define xlfFreadln 135
#define xlfFread 136
#define xlfFwriteln 137
#define xlfFwrite 138
#define xlfFpos 139
#define xlfDatevalue 140
#define xlfTimevalue 141
#define xlfSln 142
#define xlfSyd 143
#define xlfDdb 144
#define xlfGetDef 145
#define xlfReftext 146
#define xlfTextref 147
#define xlfIndirect 148
#define xlfRegister 149
#define xlfCall 150
#define xlfAddBar 151
#define xlfAddMenu 152
#define xlfAddCommand 153
#define xlfEnableCommand 154
#define xlfCheckCommand 155
#define xlfRenameCommand 156
#define xlfShowBar 157
#define xlfDeleteMenu 158
#define xlfDeleteCommand 159
#define xlfGetChartItem 160
#define xlfDialogBox 161
#define xlfClean 162
#define xlfMdeterm 163
#define xlfMinverse 164
#define xlfMmult 165
#define xlfFiles 166
#define xlfIpmt 167
#define xlfPpmt 168
#define xlfCounta 169
#define xlfCancelKey 170
#define xlfInitiate 175
#define xlfRequest 176
#define xlfPoke 177
#define xlfExecute 178
#define xlfTerminate 179
#define xlfRestart 180
#define xlfHelp 181
#define xlfGetBar 182
#define xlfProduct 183
#define xlfFact 184
#define xlfGetCell 185
#define xlfGetWorkspace 186
#define xlfGetWindow 187
#define xlfGetDocument 188
#define xlfDproduct 189
#define xlfIsnontext 190
#define xlfGetNote 191
#define xlfNote 192
#define xlfStdevp 193
#define xlfVarp 194
#define xlfDstdevp 195
#define xlfDvarp 196
#define xlfTrunc 197
#define xlfIslogical 198
#define xlfDcounta 199
#define xlfDeleteBar 200
#define xlfUnregister 201
#define xlfUsdollar 204
#define xlfFindb 205
#define xlfSearchb 206
#define xlfReplaceb 207
#define xlfLeftb 208
#define xlfRightb 209
#define xlfMidb 210
#define xlfLenb 211
#define xlfRoundup 212
#define xlfRounddown 213
#define xlfAsc 214
#define xlfDbcs 215
#define xlfRank 216
#define xlfAddress 219
#define xlfDays360 220
#define xlfToday 221
#define xlfVdb 222
#define xlfMedian 227
#define xlfSumproduct 228
#define xlfSinh 229
#define xlfCosh 230
#define xlfTanh 231
#define xlfAsinh 232
#define xlfAcosh 233
#define xlfAtanh 234
#define xlfDget 235
#define xlfCreateObject 236
#define xlfVolatile 237
#define xlfLastError 238
#define xlfCustomUndo 239
#define xlfCustomRepeat 240
#define xlfFormulaConvert 241
#define xlfGetLinkInfo 242
#define xlfTextBox 243
#define xlfInfo 244
#define xlfGroup 245
#define xlfGetObject 246
#define xlfDb 247
#define xlfPause 248
#define xlfResume 251
#define xlfFrequency 252
#define xlfAddToolbar 253
#define xlfDeleteToolbar 254
#define xlfResetToolbar 256
#define xlfEvaluate 257
#define xlfGetToolbar 258
#define xlfGetTool 259
#define xlfSpellingCheck 260
#define xlfErrorType 261
#define xlfAppTitle 262
#define xlfWindowTitle 263
#define xlfSaveToolbar 264
#define xlfEnableTool 265
#define xlfPressTool 266
#define xlfRegisterId 267
#define xlfGetWorkbook 268
#define xlfAvedev 269
#define xlfBetadist 270
#define xlfGammaln 271
#define xlfBetainv 272
#define xlfBinomdist 273
#define xlfChidist 274
#define xlfChiinv 275
#define xlfCombin 276
#define xlfConfidence 277
#define xlfCritbinom 278
#define xlfEven 279
#define xlfExpondist 280
#define xlfFdist 281
#define xlfFinv 282
#define xlfFisher 283
#define xlfFisherinv 284
#define xlfFloor 285
#define xlfGammadist 286
#define xlfGammainv 287
#define xlfCeiling 288
#define xlfHypgeomdist 289
#define xlfLognormdist 290
#define xlfLoginv 291
#define xlfNegbinomdist 292
#define xlfNormdist 293
#define xlfNormsdist 294
#define xlfNorminv 295
#define xlfNormsinv 296
#define xlfStandardize 297
#define xlfOdd 298
#define xlfPermut 299
#define xlfPoisson 300
#define xlfTdist 301
#define xlfWeibull 302
#define xlfSumxmy2 303
#define xlfSumx2my2 304
#define xlfSumx2py2 305
#define xlfChitest 306
#define xlfCorrel 307
#define xlfCovar 308
#define xlfForecast 309
#define xlfFtest 310
#define xlfIntercept 311
#define xlfPearson 312
#define xlfRsq 313
#define xlfSteyx 314
#define xlfSlope 315
#define xlfTtest 316
#define xlfProb 317
#define xlfDevsq 318
#define xlfGeomean 319
#define xlfHarmean 320
#define xlfSumsq 321
#define xlfKurt 322
#define xlfSkew 323
#define xlfZtest 324
#define xlfLarge 325
#define xlfSmall 326
#define xlfQuartile 327
#define xlfPercentile 328
#define xlfPercentrank 329
#define xlfMode 330
#define xlfTrimmean 331
#define xlfTinv 332
#define xlfMovieCommand 334
#define xlfGetMovie 335
#define xlfConcatenate 336
#define xlfPower 337
#define xlfPivotAddData 338
#define xlfGetPivotTable 339
#define xlfGetPivotField 340
#define xlfGetPivotItem 341
#define xlfRadians 342
#define xlfDegrees 343
#define xlfSubtotal 344
#define xlfSumif 345
#define xlfCountif 346
#define xlfCountblank 347
#define xlfScenarioGet 348
#define xlfOptionsListsGet 349
#define xlfIspmt 350
#define xlfDatedif 351
#define xlfDatestring 352
#define xlfNumberstring 353
#define xlfRoman 354
#define xlfOpenDialog 355
#define xlfSaveDialog 356
#define xlfViewGet 357
#define xlfGetPivotData 358
#define xlfHyperlink 359
#define xlfPhonetic 360
#define xlfAverageA 361
#define xlfMaxA 362
#define xlfMinA 363
#define xlfStDevPA 364
#define xlfVarPA 365
#define xlfStDevA 366
#define xlfVarA 367
	/* [i_a] since Excel 2007: */
#define xlfBahttext 368
#define xlfThaidayofweek 369
#define xlfThaidigit 370
#define xlfThaimonthofyear 371
#define xlfThainumsound 372
#define xlfThainumstring 373
#define xlfThaistringlength 374
#define xlfIsthaidigit 375
#define xlfRoundbahtdown 376
#define xlfRoundbahtup 377
#define xlfThaiyear 378
#define xlfRtd 379
#define xlfCubevalue 380
#define xlfCubemember 381
#define xlfCubememberproperty 382
#define xlfCuberankedmember 383
#define xlfHex2bin 384
#define xlfHex2dec 385
#define xlfHex2oct 386
#define xlfDec2bin 387
#define xlfDec2hex 388
#define xlfDec2oct 389
#define xlfOct2bin 390
#define xlfOct2hex 391
#define xlfOct2dec 392
#define xlfBin2dec 393
#define xlfBin2oct 394
#define xlfBin2hex 395
#define xlfImsub 396
#define xlfImdiv 397
#define xlfImpower 398
#define xlfImabs 399
#define xlfImsqrt 400
#define xlfImln 401
#define xlfImlog2 402
#define xlfImlog10 403
#define xlfImsin 404
#define xlfImcos 405
#define xlfImexp 406
#define xlfImargument 407
#define xlfImconjugate 408
#define xlfImaginary 409
#define xlfImreal 410
#define xlfComplex 411
#define xlfImsum 412
#define xlfImproduct 413
#define xlfSeriessum 414
#define xlfFactdouble 415
#define xlfSqrtpi 416
#define xlfQuotient 417
#define xlfDelta 418
#define xlfGestep 419
#define xlfIseven 420
#define xlfIsodd 421
#define xlfMround 422
#define xlfErf 423
#define xlfErfc 424
#define xlfBesselj 425
#define xlfBesselk 426
#define xlfBessely 427
#define xlfBesseli 428
#define xlfXirr 429
#define xlfXnpv 430
#define xlfPricemat 431
#define xlfYieldmat 432
#define xlfIntrate 433
#define xlfReceived 434
#define xlfDisc 435
#define xlfPricedisc 436
#define xlfYielddisc 437
#define xlfTbilleq 438
#define xlfTbillprice 439
#define xlfTbillyield 440
#define xlfPrice 441
#define xlfYield 442
#define xlfDollarde 443
#define xlfDollarfr 444
#define xlfNominal 445
#define xlfEffect 446
#define xlfCumprinc 447
#define xlfCumipmt 448
#define xlfEdate 449
#define xlfEomonth 450
#define xlfYearfrac 451
#define xlfCoupdaybs 452
#define xlfCoupdays 453
#define xlfCoupdaysnc 454
#define xlfCoupncd 455
#define xlfCoupnum 456
#define xlfCouppcd 457
#define xlfDuration 458
#define xlfMduration 459
#define xlfOddlprice 460
#define xlfOddlyield 461
#define xlfOddfprice 462
#define xlfOddfyield 463
#define xlfRandbetween 464
#define xlfWeeknum 465
#define xlfAmordegrc 466
#define xlfAmorlinc 467
#define xlfConvert 468
#define xlfAccrint 469
#define xlfAccrintm 470
#define xlfWorkday 471
#define xlfNetworkdays 472
#define xlfGcd 473
#define xlfMultinomial 474
#define xlfLcm 475
#define xlfFvschedule 476
#define xlfCubekpimember 477
#define xlfCubeset 478
#define xlfCubesetcount 479
#define xlfIferror 480
#define xlfCountifs 481
#define xlfSumifs 482
#define xlfAverageif 483
#define xlfAverageifs 484
	// since Excel 2010:
#define xlfAggregate 485
#define xlfBinom_dist 486
#define xlfBinom_inv 487
#define xlfConfidence_norm 488
#define xlfConfidence_t 489
#define xlfChisq_test 490
#define xlfF_test 491
#define xlfCovariance_p 492
#define xlfCovariance_s 493
#define xlfExpon_dist 494
#define xlfGamma_dist 495
#define xlfGamma_inv 496
#define xlfMode_mult 497
#define xlfMode_sngl 498
#define xlfNorm_dist 499
#define xlfNorm_inv 500
#define xlfPercentile_exc 501
#define xlfPercentile_inc 502
#define xlfPercentrank_exc 503
#define xlfPercentrank_inc 504
#define xlfPoisson_dist 505
#define xlfQuartile_exc 506
#define xlfQuartile_inc 507
#define xlfRank_avg 508
#define xlfRank_eq 509
#define xlfStdev_s 510
#define xlfStdev_p 511
#define xlfT_dist 512
#define xlfT_dist_2t 513
#define xlfT_dist_rt 514
#define xlfT_inv 515
#define xlfT_inv_2t 516
#define xlfVar_s 517
#define xlfVar_p 518
#define xlfWeibull_dist 519
#define xlfNetworkdays_intl 520
#define xlfWorkday_intl 521
#define xlfEcma_ceiling 522
#define xlfIso_ceiling 523
#define xlfBeta_dist 525
#define xlfBeta_inv 526
#define xlfChisq_dist 527
#define xlfChisq_dist_rt 528
#define xlfChisq_inv 529
#define xlfChisq_inv_rt 530
#define xlfF_dist 531
#define xlfF_dist_rt 532
#define xlfF_inv 533
#define xlfF_inv_rt 534
#define xlfHypgeom_dist 535
#define xlfLognorm_dist 536
#define xlfLognorm_inv 537
#define xlfNegbinom_dist 538
#define xlfNorm_s_dist 539
#define xlfNorm_s_inv 540
#define xlfT_test 541
#define xlfZ_test 542
#define xlfErf_precise 543
#define xlfErfc_precise 544
#define xlfGammaln_precise 545
#define xlfCeiling_precise 546
#define xlfFloor_precise 547

#endif
// XLCALL.H not loaded

	enum expr_function_code_t
	{
		/*
		 * User defined function
		 *
		 * First argument should be a function reference.
		 */
		FUNC_UDF = xlUDF,

		/*
		 * Built-in Excel functions and command equivalents
		 */
		FUNC_COUNT = xlfCount,
		FUNC_IF = xlfIf,
		FUNC_ISNA = xlfIsna,
		FUNC_ISERROR = xlfIserror,
		FUNC_SUM = xlfSum,
		FUNC_AVERAGE = xlfAverage,
		FUNC_MIN = xlfMin,
		FUNC_MAX = xlfMax,
		FUNC_ROW = xlfRow,
		FUNC_COLUMN = xlfColumn,
		FUNC_NA = xlfNa,
		FUNC_NPV = xlfNpv,
		FUNC_STDEV = xlfStdev,
		FUNC_DOLLAR = xlfDollar,
		FUNC_FIXED = xlfFixed,
		FUNC_SIN = xlfSin,
		FUNC_COS = xlfCos,
		FUNC_TAN = xlfTan,
		FUNC_ATAN = xlfAtan,
		FUNC_PI = xlfPi,
		FUNC_SQRT = xlfSqrt,
		FUNC_EXP = xlfExp,
		FUNC_LN = xlfLn,
		FUNC_LOG10 = xlfLog10,
		FUNC_ABS = xlfAbs,
		FUNC_INT = xlfInt,
		FUNC_SIGN = xlfSign,
		FUNC_ROUND = xlfRound,
		FUNC_LOOKUP = xlfLookup,
		FUNC_INDEX = xlfIndex,
		FUNC_REPT = xlfRept,
		FUNC_MID = xlfMid,
		FUNC_LEN = xlfLen,
		FUNC_VALUE = xlfValue,
		FUNC_TRUE = xlfTrue,
		FUNC_FALSE = xlfFalse,
		FUNC_AND = xlfAnd,
		FUNC_OR = xlfOr,
		FUNC_NOT = xlfNot,
		FUNC_MOD = xlfMod,
		FUNC_DCOUNT = xlfDcount,
		FUNC_DSUM = xlfDsum,
		FUNC_DAVERAGE = xlfDaverage,
		FUNC_DMIN = xlfDmin,
		FUNC_DMAX = xlfDmax,
		FUNC_DSTDEV = xlfDstdev,
		FUNC_VAR = xlfVar,
		FUNC_DVAR = xlfDvar,
		FUNC_TEXT = xlfText,
		FUNC_LINEST = xlfLinest,
		FUNC_TREND = xlfTrend,
		FUNC_LOGEST = xlfLogest,
		FUNC_GROWTH = xlfGrowth,
		FUNC_GOTO = xlfGoto,
		FUNC_HALT = xlfHalt,
		FUNC_PV = xlfPv,
		FUNC_FV = xlfFv,
		FUNC_NPER = xlfNper,
		FUNC_PMT = xlfPmt,
		FUNC_RATE = xlfRate,
		FUNC_MIRR = xlfMirr,
		FUNC_IRR = xlfIrr,
		FUNC_RAND = xlfRand,
		FUNC_MATCH = xlfMatch,
		FUNC_DATE = xlfDate,
		FUNC_TIME = xlfTime,
		FUNC_DAY = xlfDay,
		FUNC_MONTH = xlfMonth,
		FUNC_YEAR = xlfYear,
		FUNC_WEEKDAY = xlfWeekday,
		FUNC_HOUR = xlfHour,
		FUNC_MINUTE = xlfMinute,
		FUNC_SECOND = xlfSecond,
		FUNC_NOW = xlfNow,
		FUNC_AREAS = xlfAreas,
		FUNC_ROWS = xlfRows,
		FUNC_COLUMNS = xlfColumns,
		FUNC_OFFSET = xlfOffset,
		FUNC_ABSREF = xlfAbsref,
		FUNC_RELREF = xlfRelref,
		FUNC_ARGUMENT = xlfArgument,
		FUNC_SEARCH = xlfSearch,
		FUNC_TRANSPOSE = xlfTranspose,
		FUNC_ERROR = xlfError,
		FUNC_STEP = xlfStep,
		FUNC_TYPE = xlfType,
		FUNC_ECHO = xlfEcho,
		FUNC_SETNAME = xlfSetName,
		FUNC_CALLER = xlfCaller,
		FUNC_DEREF = xlfDeref,
		FUNC_WINDOWS = xlfWindows,
		FUNC_SERIES = xlfSeries,
		FUNC_DOCUMENTS = xlfDocuments,
		FUNC_ACTIVECELL = xlfActiveCell,
		FUNC_SELECTION = xlfSelection,
		FUNC_RESULT = xlfResult,
		FUNC_ATAN2 = xlfAtan2,
		FUNC_ASIN = xlfAsin,
		FUNC_ACOS = xlfAcos,
		FUNC_CHOOSE = xlfChoose,
		FUNC_HLOOKUP = xlfHlookup,
		FUNC_VLOOKUP = xlfVlookup,
		FUNC_LINKS = xlfLinks,
		FUNC_INPUT = xlfInput,
		FUNC_ISREF = xlfIsref,
		FUNC_GETFORMULA = xlfGetFormula,
		FUNC_GETNAME = xlfGetName,
		FUNC_SETVALUE = xlfSetValue,
		FUNC_LOG = xlfLog,
		FUNC_EXEC = xlfExec,
		FUNC_CHAR = xlfChar,
		FUNC_LOWER = xlfLower,
		FUNC_UPPER = xlfUpper,
		FUNC_PROPER = xlfProper,
		FUNC_LEFT = xlfLeft,
		FUNC_RIGHT = xlfRight,
		FUNC_EXACT = xlfExact,
		FUNC_TRIM = xlfTrim,
		FUNC_REPLACE = xlfReplace,
		FUNC_SUBSTITUTE = xlfSubstitute,
		FUNC_CODE = xlfCode,
		FUNC_NAMES = xlfNames,
		FUNC_DIRECTORY = xlfDirectory,
		FUNC_FIND = xlfFind,
		FUNC_CELL = xlfCell,
		FUNC_ISERR = xlfIserr,
		FUNC_ISTEXT = xlfIstext,
		FUNC_ISNUMBER = xlfIsnumber,
		FUNC_ISBLANK = xlfIsblank,
		FUNC_T = xlfT,
		FUNC_N = xlfN,
		FUNC_FOPEN = xlfFopen,
		FUNC_FCLOSE = xlfFclose,
		FUNC_FSIZE = xlfFsize,
		FUNC_FREADLN = xlfFreadln,
		FUNC_FREAD = xlfFread,
		FUNC_FWRITELN = xlfFwriteln,
		FUNC_FWRITE = xlfFwrite,
		FUNC_FPOS = xlfFpos,
		FUNC_DATEVALUE = xlfDatevalue,
		FUNC_TIMEVALUE = xlfTimevalue,
		FUNC_SLN = xlfSln,
		FUNC_SYD = xlfSyd,
		FUNC_DDB = xlfDdb,
		FUNC_GETDEF = xlfGetDef,
		FUNC_REFTEXT = xlfReftext,
		FUNC_TEXTREF = xlfTextref,
		FUNC_INDIRECT = xlfIndirect,
		FUNC_REGISTER = xlfRegister,
		FUNC_CALL = xlfCall,
		FUNC_ADDBAR = xlfAddBar,
		FUNC_ADDMENU = xlfAddMenu,
		FUNC_ADDCOMMAND = xlfAddCommand,
		FUNC_ENABLECOMMAND = xlfEnableCommand,
		FUNC_CHECKCOMMAND = xlfCheckCommand,
		FUNC_RENAMECOMMAND = xlfRenameCommand,
		FUNC_SHOWBAR = xlfShowBar,
		FUNC_DELETEMENU = xlfDeleteMenu,
		FUNC_DELETECOMMAND = xlfDeleteCommand,
		FUNC_GETCHARTITEM = xlfGetChartItem,
		FUNC_DIALOGBOX = xlfDialogBox,
		FUNC_CLEAN = xlfClean,
		FUNC_MDETERM = xlfMdeterm,
		FUNC_MINVERSE = xlfMinverse,
		FUNC_MMULT = xlfMmult,
		FUNC_FILES = xlfFiles,
		FUNC_IPMT = xlfIpmt,
		FUNC_PPMT = xlfPpmt,
		FUNC_COUNTA = xlfCounta,
		FUNC_CANCELKEY = xlfCancelKey,
		FUNC_INITIATE = xlfInitiate,
		FUNC_REQUEST = xlfRequest,
		FUNC_POKE = xlfPoke,
		FUNC_EXECUTE = xlfExecute,
		FUNC_TERMINATE = xlfTerminate,
		FUNC_RESTART = xlfRestart,
		FUNC_HELP = xlfHelp,
		FUNC_GETBAR = xlfGetBar,
		FUNC_PRODUCT = xlfProduct,
		FUNC_FACT = xlfFact,
		FUNC_GETCELL = xlfGetCell,
		FUNC_GETWORKSPACE = xlfGetWorkspace,
		FUNC_GETWINDOW = xlfGetWindow,
		FUNC_GETDOCUMENT = xlfGetDocument,
		FUNC_DPRODUCT = xlfDproduct,
		FUNC_ISNONTEXT = xlfIsnontext,
		FUNC_GETNOTE = xlfGetNote,
		FUNC_NOTE = xlfNote,
		FUNC_STDEVP = xlfStdevp,
		FUNC_VARP = xlfVarp,
		FUNC_DSTDEVP = xlfDstdevp,
		FUNC_DVARP = xlfDvarp,
		FUNC_TRUNC = xlfTrunc,
		FUNC_ISLOGICAL = xlfIslogical,
		FUNC_DCOUNTA = xlfDcounta,
		FUNC_DELETEBAR = xlfDeleteBar,
		FUNC_UNREGISTER = xlfUnregister,
		FUNC_USDOLLAR = xlfUsdollar,
		FUNC_FINDB = xlfFindb,
		FUNC_SEARCHB = xlfSearchb,
		FUNC_REPLACEB = xlfReplaceb,
		FUNC_LEFTB = xlfLeftb,
		FUNC_RIGHTB = xlfRightb,
		FUNC_MIDB = xlfMidb,
		FUNC_LENB = xlfLenb,
		FUNC_ROUNDUP = xlfRoundup,
		FUNC_ROUNDDOWN = xlfRounddown,
		FUNC_ASC = xlfAsc,
		FUNC_DBCS = xlfDbcs,
		FUNC_RANK = xlfRank,
		FUNC_ADDRESS = xlfAddress,
		FUNC_DAYS360 = xlfDays360,
		FUNC_TODAY = xlfToday,
		FUNC_VDB = xlfVdb,
		FUNC_MEDIAN = xlfMedian,
		FUNC_SUMPRODUCT = xlfSumproduct,
		FUNC_SINH = xlfSinh,
		FUNC_COSH = xlfCosh,
		FUNC_TANH = xlfTanh,
		FUNC_ASINH = xlfAsinh,
		FUNC_ACOSH = xlfAcosh,
		FUNC_ATANH = xlfAtanh,
		FUNC_DGET = xlfDget,
		FUNC_CREATEOBJECT = xlfCreateObject,
		FUNC_VOLATILE = xlfVolatile,
		FUNC_LASTERROR = xlfLastError,
		FUNC_CUSTOMUNDO = xlfCustomUndo,
		FUNC_CUSTOMREPEAT = xlfCustomRepeat,
		FUNC_FORMULACONVERT = xlfFormulaConvert,
		FUNC_GETLINKINFO = xlfGetLinkInfo,
		FUNC_TEXTBOX = xlfTextBox,
		FUNC_INFO = xlfInfo,
		FUNC_GROUP = xlfGroup,
		FUNC_GETOBJECT = xlfGetObject,
		FUNC_DB = xlfDb,
		FUNC_PAUSE = xlfPause,
		FUNC_RESUME = xlfResume,
		FUNC_FREQUENCY = xlfFrequency,
		FUNC_ADDTOOLBAR = xlfAddToolbar,
		FUNC_DELETETOOLBAR = xlfDeleteToolbar,
		FUNC_RESETTOOLBAR = xlfResetToolbar,
		FUNC_EVALUATE = xlfEvaluate,
		FUNC_GETTOOLBAR = xlfGetToolbar,
		FUNC_GETTOOL = xlfGetTool,
		FUNC_SPELLINGCHECK = xlfSpellingCheck,
		FUNC_ERRORTYPE = xlfErrorType,
		FUNC_APPTITLE = xlfAppTitle,
		FUNC_WINDOWTITLE = xlfWindowTitle,
		FUNC_SAVETOOLBAR = xlfSaveToolbar,
		FUNC_ENABLETOOL = xlfEnableTool,
		FUNC_PRESSTOOL = xlfPressTool,
		FUNC_REGISTERID = xlfRegisterId,
		FUNC_GETWORKBOOK = xlfGetWorkbook,
		FUNC_AVEDEV = xlfAvedev,
		FUNC_BETADIST = xlfBetadist,
		FUNC_GAMMALN = xlfGammaln,
		FUNC_BETAINV = xlfBetainv,
		FUNC_BINOMDIST = xlfBinomdist,
		FUNC_CHIDIST = xlfChidist,
		FUNC_CHIINV = xlfChiinv,
		FUNC_COMBIN = xlfCombin,
		FUNC_CONFIDENCE = xlfConfidence,
		FUNC_CRITBINOM = xlfCritbinom,
		FUNC_EVEN = xlfEven,
		FUNC_EXPONDIST = xlfExpondist,
		FUNC_FDIST = xlfFdist,
		FUNC_FINV = xlfFinv,
		FUNC_FISHER = xlfFisher,
		FUNC_FISHERINV = xlfFisherinv,
		FUNC_FLOOR = xlfFloor,
		FUNC_GAMMADIST = xlfGammadist,
		FUNC_GAMMAINV = xlfGammainv,
		FUNC_CEILING = xlfCeiling,
		FUNC_HYPGEOMDIST = xlfHypgeomdist,
		FUNC_LOGNORMDIST = xlfLognormdist,
		FUNC_LOGINV = xlfLoginv,
		FUNC_NEGBINOMDIST = xlfNegbinomdist,
		FUNC_NORMDIST = xlfNormdist,
		FUNC_NORMSDIST = xlfNormsdist,
		FUNC_NORMINV = xlfNorminv,
		FUNC_NORMSINV = xlfNormsinv,
		FUNC_STANDARDIZE = xlfStandardize,
		FUNC_ODD = xlfOdd,
		FUNC_PERMUT = xlfPermut,
		FUNC_POISSON = xlfPoisson,
		FUNC_TDIST = xlfTdist,
		FUNC_WEIBULL = xlfWeibull,
		FUNC_SUMXMY2 = xlfSumxmy2,
		FUNC_SUMX2MY2 = xlfSumx2my2,
		FUNC_SUMX2PY2 = xlfSumx2py2,
		FUNC_CHITEST = xlfChitest,
		FUNC_CORREL = xlfCorrel,
		FUNC_COVAR = xlfCovar,
		FUNC_FORECAST = xlfForecast,
		FUNC_FTEST = xlfFtest,
		FUNC_INTERCEPT = xlfIntercept,
		FUNC_PEARSON = xlfPearson,
		FUNC_RSQ = xlfRsq,
		FUNC_STEYX = xlfSteyx,
		FUNC_SLOPE = xlfSlope,
		FUNC_TTEST = xlfTtest,
		FUNC_PROB = xlfProb,
		FUNC_DEVSQ = xlfDevsq,
		FUNC_GEOMEAN = xlfGeomean,
		FUNC_HARMEAN = xlfHarmean,
		FUNC_SUMSQ = xlfSumsq,
		FUNC_KURT = xlfKurt,
		FUNC_SKEW = xlfSkew,
		FUNC_ZTEST = xlfZtest,
		FUNC_LARGE = xlfLarge,
		FUNC_SMALL = xlfSmall,
		FUNC_QUARTILE = xlfQuartile,
		FUNC_PERCENTILE = xlfPercentile,
		FUNC_PERCENTRANK = xlfPercentrank,
		FUNC_MODE = xlfMode,
		FUNC_TRIMMEAN = xlfTrimmean,
		FUNC_TINV = xlfTinv,
		FUNC_MOVIECOMMAND = xlfMovieCommand,
		FUNC_GETMOVIE = xlfGetMovie,
		FUNC_CONCATENATE = xlfConcatenate,
		FUNC_POWER = xlfPower,
		FUNC_PIVOTADDDATA = xlfPivotAddData,
		FUNC_GETPIVOTTABLE = xlfGetPivotTable,
		FUNC_GETPIVOTFIELD = xlfGetPivotField,
		FUNC_GETPIVOTITEM = xlfGetPivotItem,
		FUNC_RADIANS = xlfRadians,
		FUNC_DEGREES = xlfDegrees,
		FUNC_SUBTOTAL = xlfSubtotal,
		FUNC_SUMIF = xlfSumif,
		FUNC_COUNTIF = xlfCountif,
		FUNC_COUNTBLANK = xlfCountblank,
		FUNC_SCENARIOGET = xlfScenarioGet,
		FUNC_OPTIONSLISTSGET = xlfOptionsListsGet,
		FUNC_ISPMT = xlfIspmt,
		FUNC_DATEDIF = xlfDatedif,
		FUNC_DATESTRING = xlfDatestring,
		FUNC_NUMBERSTRING = xlfNumberstring,
		FUNC_ROMAN = xlfRoman,
		FUNC_OPENDIALOG = xlfOpenDialog,
		FUNC_SAVEDIALOG = xlfSaveDialog,
		FUNC_VIEWGET = xlfViewGet,
		FUNC_GETPIVOTDATA = xlfGetPivotData,
		FUNC_HYPERLINK = xlfHyperlink,
		FUNC_PHONETIC = xlfPhonetic,
		FUNC_AVERAGEA = xlfAverageA,
		FUNC_MAXA = xlfMaxA,
		FUNC_MINA = xlfMinA,
		FUNC_STDEVPA = xlfStDevPA,
		FUNC_VARPA = xlfVarPA,
		FUNC_STDEVA = xlfStDevA,
		FUNC_VARA = xlfVarA,
		FUNC_BAHTTEXT = xlfBahttext,
		FUNC_THAIDAYOFWEEK = xlfThaidayofweek,
		FUNC_THAIDIGIT = xlfThaidigit,
		FUNC_THAIMONTHOFYEAR = xlfThaimonthofyear,
		FUNC_THAINUMSOUND = xlfThainumsound,
		FUNC_THAINUMSTRING = xlfThainumstring,
		FUNC_THAISTRINGLENGTH = xlfThaistringlength,
		FUNC_ISTHAIDIGIT = xlfIsthaidigit,
		FUNC_ROUNDBAHTDOWN = xlfRoundbahtdown,
		FUNC_ROUNDBAHTUP = xlfRoundbahtup,
		FUNC_THAIYEAR = xlfThaiyear,
		FUNC_RTD = xlfRtd,
		FUNC_CUBEVALUE = xlfCubevalue,
		FUNC_CUBEMEMBER = xlfCubemember,
		FUNC_CUBEMEMBERPROPERTY = xlfCubememberproperty,
		FUNC_CUBERANKEDMEMBER = xlfCuberankedmember,
		FUNC_HEX2BIN = xlfHex2bin,
		FUNC_HEX2DEC = xlfHex2dec,
		FUNC_HEX2OCT = xlfHex2oct,
		FUNC_DEC2BIN = xlfDec2bin,
		FUNC_DEC2HEX = xlfDec2hex,
		FUNC_DEC2OCT = xlfDec2oct,
		FUNC_OCT2BIN = xlfOct2bin,
		FUNC_OCT2HEX = xlfOct2hex,
		FUNC_OCT2DEC = xlfOct2dec,
		FUNC_BIN2DEC = xlfBin2dec,
		FUNC_BIN2OCT = xlfBin2oct,
		FUNC_BIN2HEX = xlfBin2hex,
		FUNC_IMSUB = xlfImsub,
		FUNC_IMDIV = xlfImdiv,
		FUNC_IMPOWER = xlfImpower,
		FUNC_IMABS = xlfImabs,
		FUNC_IMSQRT = xlfImsqrt,
		FUNC_IMLN = xlfImln,
		FUNC_IMLOG2 = xlfImlog2,
		FUNC_IMLOG10 = xlfImlog10,
		FUNC_IMSIN = xlfImsin,
		FUNC_IMCOS = xlfImcos,
		FUNC_IMEXP = xlfImexp,
		FUNC_IMARGUMENT = xlfImargument,
		FUNC_IMCONJUGATE = xlfImconjugate,
		FUNC_IMAGINARY = xlfImaginary,
		FUNC_IMREAL = xlfImreal,
		FUNC_COMPLEX = xlfComplex,
		FUNC_IMSUM = xlfImsum,
		FUNC_IMPRODUCT = xlfImproduct,
		FUNC_SERIESSUM = xlfSeriessum,
		FUNC_FACTDOUBLE = xlfFactdouble,
		FUNC_SQRTPI = xlfSqrtpi,
		FUNC_QUOTIENT = xlfQuotient,
		FUNC_DELTA = xlfDelta,
		FUNC_GESTEP = xlfGestep,
		FUNC_ISEVEN = xlfIseven,
		FUNC_ISODD = xlfIsodd,
		FUNC_MROUND = xlfMround,
		FUNC_ERF = xlfErf,
		FUNC_ERFC = xlfErfc,
		FUNC_BESSELJ = xlfBesselj,
		FUNC_BESSELK = xlfBesselk,
		FUNC_BESSELY = xlfBessely,
		FUNC_BESSELI = xlfBesseli,
		FUNC_XIRR = xlfXirr,
		FUNC_XNPV = xlfXnpv,
		FUNC_PRICEMAT = xlfPricemat,
		FUNC_YIELDMAT = xlfYieldmat,
		FUNC_INTRATE = xlfIntrate,
		FUNC_RECEIVED = xlfReceived,
		FUNC_DISC = xlfDisc,
		FUNC_PRICEDISC = xlfPricedisc,
		FUNC_YIELDDISC = xlfYielddisc,
		FUNC_TBILLEQ = xlfTbilleq,
		FUNC_TBILLPRICE = xlfTbillprice,
		FUNC_TBILLYIELD = xlfTbillyield,
		FUNC_PRICE = xlfPrice,
		FUNC_YIELD = xlfYield,
		FUNC_DOLLARDE = xlfDollarde,
		FUNC_DOLLARFR = xlfDollarfr,
		FUNC_NOMINAL = xlfNominal,
		FUNC_EFFECT = xlfEffect,
		FUNC_CUMPRINC = xlfCumprinc,
		FUNC_CUMIPMT = xlfCumipmt,
		FUNC_EDATE = xlfEdate,
		FUNC_EOMONTH = xlfEomonth,
		FUNC_YEARFRAC = xlfYearfrac,
		FUNC_COUPDAYBS = xlfCoupdaybs,
		FUNC_COUPDAYS = xlfCoupdays,
		FUNC_COUPDAYSNC = xlfCoupdaysnc,
		FUNC_COUPNCD = xlfCoupncd,
		FUNC_COUPNUM = xlfCoupnum,
		FUNC_COUPPCD = xlfCouppcd,
		FUNC_DURATION = xlfDuration,
		FUNC_MDURATION = xlfMduration,
		FUNC_ODDLPRICE = xlfOddlprice,
		FUNC_ODDLYIELD = xlfOddlyield,
		FUNC_ODDFPRICE = xlfOddfprice,
		FUNC_ODDFYIELD = xlfOddfyield,
		FUNC_RANDBETWEEN = xlfRandbetween,
		FUNC_WEEKNUM = xlfWeeknum,
		FUNC_AMORDEGRC = xlfAmordegrc,
		FUNC_AMORLINC = xlfAmorlinc,
		FUNC_CONVERT = xlfConvert,
		FUNC_ACCRINT = xlfAccrint,
		FUNC_ACCRINTM = xlfAccrintm,
		FUNC_WORKDAY = xlfWorkday,
		FUNC_NETWORKDAYS = xlfNetworkdays,
		FUNC_GCD = xlfGcd,
		FUNC_MULTINOMIAL = xlfMultinomial,
		FUNC_LCM = xlfLcm,
		FUNC_FVSCHEDULE = xlfFvschedule,
		FUNC_CUBEKPIMEMBER = xlfCubekpimember,
		FUNC_CUBESET = xlfCubeset,
		FUNC_CUBESETCOUNT = xlfCubesetcount,
		FUNC_IFERROR = xlfIferror,
		FUNC_COUNTIFS = xlfCountifs,
		FUNC_SUMIFS = xlfSumifs,
		FUNC_AVERAGEIF = xlfAverageif,
		FUNC_AVERAGEIFS = xlfAverageifs,
		FUNC_AGGREGATE = xlfAggregate,
		FUNC_BINOM_DIST = xlfBinom_dist,
		FUNC_BINOM_INV = xlfBinom_inv,
		FUNC_CONFIDENCE_NORM = xlfConfidence_norm,
		FUNC_CONFIDENCE_T = xlfConfidence_t,
		FUNC_CHISQ_TEST = xlfChisq_test,
		FUNC_F_TEST = xlfF_test,
		FUNC_COVARIANCE_P = xlfCovariance_p,
		FUNC_COVARIANCE_S = xlfCovariance_s,
		FUNC_EXPON_DIST = xlfExpon_dist,
		FUNC_GAMMA_DIST = xlfGamma_dist,
		FUNC_GAMMA_INV = xlfGamma_inv,
		FUNC_MODE_MULT = xlfMode_mult,
		FUNC_MODE_SNGL = xlfMode_sngl,
		FUNC_NORM_DIST = xlfNorm_dist,
		FUNC_NORM_INV = xlfNorm_inv,
		FUNC_PERCENTILE_EXC = xlfPercentile_exc,
		FUNC_PERCENTILE_INC = xlfPercentile_inc,
		FUNC_PERCENTRANK_EXC = xlfPercentrank_exc,
		FUNC_PERCENTRANK_INC = xlfPercentrank_inc,
		FUNC_POISSON_DIST = xlfPoisson_dist,
		FUNC_QUARTILE_EXC = xlfQuartile_exc,
		FUNC_QUARTILE_INC = xlfQuartile_inc,
		FUNC_RANK_AVG = xlfRank_avg,
		FUNC_RANK_EQ = xlfRank_eq,
		FUNC_STDEV_S = xlfStdev_s,
		FUNC_STDEV_P = xlfStdev_p,
		FUNC_T_DIST = xlfT_dist,
		FUNC_T_DIST_2T = xlfT_dist_2t,
		FUNC_T_DIST_RT = xlfT_dist_rt,
		FUNC_T_INV = xlfT_inv,
		FUNC_T_INV_2T = xlfT_inv_2t,
		FUNC_VAR_S = xlfVar_s,
		FUNC_VAR_P = xlfVar_p,
		FUNC_WEIBULL_DIST = xlfWeibull_dist,
		FUNC_NETWORKDAYS_INTL = xlfNetworkdays_intl,
		FUNC_WORKDAY_INTL = xlfWorkday_intl,
		FUNC_ECMA_CEILING = xlfEcma_ceiling,
		FUNC_ISO_CEILING = xlfIso_ceiling,
		FUNC_BETA_DIST = xlfBeta_dist,
		FUNC_BETA_INV = xlfBeta_inv,
		FUNC_CHISQ_DIST = xlfChisq_dist,
		FUNC_CHISQ_DIST_RT = xlfChisq_dist_rt,
		FUNC_CHISQ_INV = xlfChisq_inv,
		FUNC_CHISQ_INV_RT = xlfChisq_inv_rt,
		FUNC_F_DIST = xlfF_dist,
		FUNC_F_DIST_RT = xlfF_dist_rt,
		FUNC_F_INV = xlfF_inv,
		FUNC_F_INV_RT = xlfF_inv_rt,
		FUNC_HYPGEOM_DIST = xlfHypgeom_dist,
		FUNC_LOGNORM_DIST = xlfLognorm_dist,
		FUNC_LOGNORM_INV = xlfLognorm_inv,
		FUNC_NEGBINOM_DIST = xlfNegbinom_dist,
		FUNC_NORM_S_DIST = xlfNorm_s_dist,
		FUNC_NORM_S_INV = xlfNorm_s_inv,
		FUNC_T_TEST = xlfT_test,
		FUNC_Z_TEST = xlfZ_test,
		FUNC_ERF_PRECISE = xlfErf_precise,
		FUNC_ERFC_PRECISE = xlfErfc_precise,
		FUNC_GAMMALN_PRECISE = xlfGammaln_precise,
		FUNC_CEILING_PRECISE = xlfCeiling_precise,
		FUNC_FLOOR_PRECISE = xlfFloor_precise,
	};
}

#endif
