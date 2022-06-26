// 
// Copyright (C) 2001 Leptonica.  All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided with the distribution.
// 
#ifndef  LEPTONICA_ALLHEADERS_H
#define  LEPTONICA_ALLHEADERS_H

#define LIBLEPT_MAJOR_VERSION   1
#define LIBLEPT_MINOR_VERSION   82
#define LIBLEPT_PATCH_VERSION   0

#ifdef HAVE_CONFIG_H
	#include <config_auto.h>
#endif
#include <slib.h>
#include "alltypes.h"

#ifndef NO_PROTOS
/*
 *  These prototypes were autogen'd by xtractprotos, v. 1.5
 */
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

LEPT_DLL extern PIX * pixCleanBackgroundToWhite(PIX * pixs, PIX * pixim, PIX * pixg, float gamma, int32 blackval, int32 whiteval);
LEPT_DLL extern PIX * pixBackgroundNormSimple(PIX * pixs, PIX * pixim, PIX * pixg);
LEPT_DLL extern PIX * pixBackgroundNorm(PIX * pixs, PIX * pixim, PIX * pixg, int32 sx, int32 sy,
    int32 thresh, int32 mincount, int32 bgval, int32 smoothx, int32 smoothy);
LEPT_DLL extern PIX * pixBackgroundNormMorph(PIX * pixs, PIX * pixim, int32 reduction, int32 size, int32 bgval);
LEPT_DLL extern l_ok pixBackgroundNormGrayArray(PIX * pixs, PIX * pixim, int32 sx, int32 sy, int32 thresh, int32 mincount,
    int32 bgval, int32 smoothx, int32 smoothy, PIX ** ppixd);
LEPT_DLL extern l_ok pixBackgroundNormRGBArrays(PIX * pixs, PIX * pixim, PIX * pixg, int32 sx, int32 sy,
    int32 thresh, int32 mincount, int32 bgval, int32 smoothx, int32 smoothy, PIX ** ppixr, PIX ** ppixg, PIX ** ppixb);
LEPT_DLL extern l_ok pixBackgroundNormGrayArrayMorph(PIX * pixs, PIX * pixim, int32 reduction, int32 size, int32 bgval, PIX ** ppixd);
LEPT_DLL extern l_ok pixBackgroundNormRGBArraysMorph(PIX * pixs,
    PIX * pixim,
    int32 reduction,
    int32 size,
    int32 bgval,
    PIX ** ppixr,
    PIX ** ppixg,
    PIX ** ppixb);
LEPT_DLL extern l_ok pixGetBackgroundGrayMap(PIX * pixs, PIX * pixim, int32 sx, int32 sy, int32 thresh, int32 mincount, PIX ** ppixd);
LEPT_DLL extern l_ok pixGetBackgroundRGBMap(PIX * pixs, PIX * pixim, PIX * pixg, int32 sx, int32 sy, int32 thresh, int32 mincount, PIX ** ppixmr, PIX ** ppixmg, PIX ** ppixmb);
LEPT_DLL extern l_ok pixGetBackgroundGrayMapMorph(PIX * pixs, PIX * pixim, int32 reduction, int32 size, PIX ** ppixm);
LEPT_DLL extern l_ok pixGetBackgroundRGBMapMorph(PIX * pixs, PIX * pixim, int32 reduction, int32 size, PIX ** ppixmr, PIX ** ppixmg, PIX ** ppixmb);
LEPT_DLL extern l_ok pixFillMapHoles(PIX * pix, int32 nx, int32 ny, int32 filltype);
LEPT_DLL extern PIX * pixExtendByReplication(PIX * pixs, int32 addw, int32 addh);
LEPT_DLL extern l_ok pixSmoothConnectedRegions(PIX * pixs, PIX * pixm, int32 factor);
LEPT_DLL extern PIX * pixGetInvBackgroundMap(PIX * pixs, int32 bgval, int32 smoothx, int32 smoothy);
LEPT_DLL extern PIX * pixApplyInvBackgroundGrayMap(PIX * pixs, PIX * pixm, int32 sx, int32 sy);
LEPT_DLL extern PIX * pixApplyInvBackgroundRGBMap(PIX * pixs, PIX * pixmr, PIX * pixmg, PIX * pixmb, int32 sx, int32 sy);
LEPT_DLL extern PIX * pixApplyVariableGrayMap(PIX * pixs, PIX * pixg, int32 target);
LEPT_DLL extern PIX * pixGlobalNormRGB(PIX * pixd, PIX * pixs, int32 rval, int32 gval, int32 bval, int32 mapval);
LEPT_DLL extern PIX * pixGlobalNormNoSatRGB(PIX * pixd, PIX * pixs, int32 rval, int32 gval, int32 bval, int32 factor, float rank);
LEPT_DLL extern l_ok pixThresholdSpreadNorm(PIX * pixs,
    int32 filtertype,
    int32 edgethresh,
    int32 smoothx,
    int32 smoothy,
    float gamma,
    int32 minval,
    int32 maxval,
    int32 targetthresh,
    PIX ** ppixth,
    PIX ** ppixb,
    PIX ** ppixd);
LEPT_DLL extern PIX * pixBackgroundNormFlex(PIX * pixs, int32 sx, int32 sy, int32 smoothx, int32 smoothy, int32 delta);
LEPT_DLL extern PIX * pixContrastNorm(PIX * pixd, PIX * pixs, int32 sx, int32 sy, int32 mindiff, int32 smoothx, int32 smoothy);
LEPT_DLL extern PIX * pixAffineSampledPta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixAffineSampled(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixAffinePta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixAffine(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixAffinePtaColor(PIX * pixs, PTA * ptad, PTA * ptas, uint32 colorval);
LEPT_DLL extern PIX * pixAffineColor(PIX * pixs, float * vc, uint32 colorval);
LEPT_DLL extern PIX * pixAffinePtaGray(PIX * pixs, PTA * ptad, PTA * ptas, uint8 grayval);
LEPT_DLL extern PIX * pixAffineGray(PIX * pixs, float * vc, uint8 grayval);
LEPT_DLL extern PIX * pixAffinePtaWithAlpha(PIX * pixs, PTA * ptad, PTA * ptas, PIX * pixg, float fract, int32 border);
LEPT_DLL extern l_ok getAffineXformCoeffs(PTA * ptas, PTA * ptad, float ** pvc);
LEPT_DLL extern l_ok affineInvertXform(float * vc, float ** pvci);
LEPT_DLL extern l_ok affineXformSampledPt(float * vc, int32 x, int32 y, int32 * pxp, int32 * pyp);
LEPT_DLL extern l_ok affineXformPt(float * vc, int32 x, int32 y, float * pxp, float * pyp);
LEPT_DLL extern l_ok linearInterpolatePixelColor(uint32 * datas, int32 wpls, int32 w, int32 h, float x, float y, uint32 colorval, uint32 * pval);
LEPT_DLL extern l_ok linearInterpolatePixelGray(uint32 * datas, int32 wpls, int32 w, int32 h, float x, float y, int32 grayval, int32 * pval);
LEPT_DLL extern int32 gaussjordan(float ** a, float * b, int32 n);
LEPT_DLL extern PIX * pixAffineSequential(PIX * pixs, PTA * ptad, PTA * ptas, int32 bw, int32 bh);
LEPT_DLL extern float * createMatrix2dTranslate(float transx, float transy);
LEPT_DLL extern float * createMatrix2dScale(float scalex, float scaley);
LEPT_DLL extern float * createMatrix2dRotate(float xc, float yc, float angle);
LEPT_DLL extern PTA * ptaTranslate(PTA * ptas, float transx, float transy);
LEPT_DLL extern PTA * ptaScale(PTA * ptas, float scalex, float scaley);
LEPT_DLL extern PTA * ptaRotate(PTA * ptas, float xc, float yc, float angle);
LEPT_DLL extern BOXA * boxaTranslate(BOXA * boxas, float transx, float transy);
LEPT_DLL extern BOXA * boxaScale(BOXA * boxas, float scalex, float scaley);
LEPT_DLL extern BOXA * boxaRotate(BOXA * boxas, float xc, float yc, float angle);
LEPT_DLL extern PTA * ptaAffineTransform(PTA * ptas, float * mat);
LEPT_DLL extern BOXA * boxaAffineTransform(BOXA * boxas, float * mat);
LEPT_DLL extern l_ok l_productMatVec(float * mat, float * vecs, float * vecd, int32 size);
LEPT_DLL extern l_ok l_productMat2(float * mat1, float * mat2, float * matd, int32 size);
LEPT_DLL extern l_ok l_productMat3(float * mat1, float * mat2, float * mat3, float * matd, int32 size);
LEPT_DLL extern l_ok l_productMat4(float * mat1, float * mat2, float * mat3, float * mat4, float * matd, int32 size);
LEPT_DLL extern int32 l_getDataBit(const void * line, int32 n);
LEPT_DLL extern void l_setDataBit(void * line, int32 n);
LEPT_DLL extern void l_clearDataBit(void * line, int32 n);
LEPT_DLL extern void l_setDataBitVal(void * line, int32 n, int32 val);
LEPT_DLL extern int32 l_getDataDibit(const void * line, int32 n);
LEPT_DLL extern void l_setDataDibit(void * line, int32 n, int32 val);
LEPT_DLL extern void l_clearDataDibit(void * line, int32 n);
LEPT_DLL extern int32 l_getDataQbit(const void * line, int32 n);
LEPT_DLL extern void l_setDataQbit(void * line, int32 n, int32 val);
LEPT_DLL extern void l_clearDataQbit(void * line, int32 n);
LEPT_DLL extern int32 l_getDataByte(const void * line, int32 n);
LEPT_DLL extern void l_setDataByte(void * line, int32 n, int32 val);
LEPT_DLL extern int32 l_getDataTwoBytes(const void * line, int32 n);
LEPT_DLL extern void l_setDataTwoBytes(void * line, int32 n, int32 val);
LEPT_DLL extern int32 l_getDataFourBytes(const void * line, int32 n);
LEPT_DLL extern void l_setDataFourBytes(void * line, int32 n, int32 val);
LEPT_DLL extern char * barcodeDispatchDecoder(char * barstr, int32 format, int32 debugflag);
LEPT_DLL extern int32 barcodeFormatIsSupported(int32 format);
LEPT_DLL extern NUMA * pixFindBaselines(PIX * pixs, PTA ** ppta, PIXA * pixadb);
LEPT_DLL extern PIX * pixDeskewLocal(PIX * pixs, int32 nslices, int32 redsweep, int32 redsearch, float sweeprange, float sweepdelta, float minbsdelta);
LEPT_DLL extern l_ok pixGetLocalSkewTransform(PIX * pixs, int32 nslices, int32 redsweep, int32 redsearch, float sweeprange, float sweepdelta, float minbsdelta, PTA ** pptas, PTA ** pptad);
LEPT_DLL extern NUMA * pixGetLocalSkewAngles(PIX * pixs, int32 nslices, int32 redsweep, int32 redsearch, float sweeprange, float sweepdelta, float minbsdelta, float * pa, float * pb, int32 debug);
LEPT_DLL extern L_BBUFFER * bbufferCreate(const uint8 * indata, int32 nalloc);
LEPT_DLL extern void bbufferDestroy(L_BBUFFER ** pbb);
LEPT_DLL extern uint8 * bbufferDestroyAndSaveData(L_BBUFFER ** pbb, size_t * pnbytes);
LEPT_DLL extern l_ok bbufferRead(L_BBUFFER * bb, uint8 * src, int32 nbytes);
LEPT_DLL extern l_ok bbufferReadStream(L_BBUFFER * bb, FILE * fp, int32 nbytes);
LEPT_DLL extern l_ok bbufferExtendArray(L_BBUFFER * bb, int32 nbytes);
LEPT_DLL extern l_ok bbufferWrite(L_BBUFFER * bb, uint8 * dest, size_t nbytes, size_t * pnout);
LEPT_DLL extern l_ok bbufferWriteStream(L_BBUFFER * bb, FILE * fp, size_t nbytes, size_t * pnout);
LEPT_DLL extern PIX * pixBilateral(PIX * pixs, float spatial_stdev, float range_stdev, int32 ncomps, int32 reduction);
LEPT_DLL extern PIX * pixBilateralGray(PIX * pixs, float spatial_stdev, float range_stdev, int32 ncomps, int32 reduction);
LEPT_DLL extern PIX * pixBilateralExact(PIX * pixs, L_KERNEL * spatial_kel, L_KERNEL * range_kel);
LEPT_DLL extern PIX * pixBilateralGrayExact(PIX * pixs, L_KERNEL * spatial_kel, L_KERNEL * range_kel);
LEPT_DLL extern PIX* pixBlockBilateralExact(PIX * pixs, float spatial_stdev, float range_stdev);
LEPT_DLL extern L_KERNEL * makeRangeKernel(float range_stdev);
LEPT_DLL extern PIX * pixBilinearSampledPta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixBilinearSampled(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixBilinearPta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixBilinear(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixBilinearPtaColor(PIX * pixs, PTA * ptad, PTA * ptas, uint32 colorval);
LEPT_DLL extern PIX * pixBilinearColor(PIX * pixs, float * vc, uint32 colorval);
LEPT_DLL extern PIX * pixBilinearPtaGray(PIX * pixs, PTA * ptad, PTA * ptas, uint8 grayval);
LEPT_DLL extern PIX * pixBilinearGray(PIX * pixs, float * vc, uint8 grayval);
LEPT_DLL extern PIX * pixBilinearPtaWithAlpha(PIX * pixs, PTA * ptad, PTA * ptas, PIX * pixg, float fract, int32 border);
LEPT_DLL extern l_ok getBilinearXformCoeffs(PTA * ptas, PTA * ptad, float ** pvc);
LEPT_DLL extern l_ok bilinearXformSampledPt(float * vc, int32 x, int32 y, int32 * pxp, int32 * pyp);
LEPT_DLL extern l_ok bilinearXformPt(float * vc, int32 x, int32 y, float * pxp, float * pyp);
LEPT_DLL extern l_ok pixOtsuAdaptiveThreshold(PIX * pixs,
    int32 sx,
    int32 sy,
    int32 smoothx,
    int32 smoothy,
    float scorefract,
    PIX ** ppixth,
    PIX ** ppixd);
LEPT_DLL extern PIX * pixOtsuThreshOnBackgroundNorm(PIX * pixs,
    PIX * pixim,
    int32 sx,
    int32 sy,
    int32 thresh,
    int32 mincount,
    int32 bgval,
    int32 smoothx,
    int32 smoothy,
    float scorefract,
    int32 * pthresh);
LEPT_DLL extern PIX * pixMaskedThreshOnBackgroundNorm(PIX * pixs,
    PIX * pixim,
    int32 sx,
    int32 sy,
    int32 thresh,
    int32 mincount,
    int32 smoothx,
    int32 smoothy,
    float scorefract,
    int32 * pthresh);
LEPT_DLL extern l_ok pixSauvolaBinarizeTiled(PIX * pixs,
    int32 whsize,
    float factor,
    int32 nx,
    int32 ny,
    PIX ** ppixth,
    PIX ** ppixd);
LEPT_DLL extern l_ok pixSauvolaBinarize(PIX * pixs,
    int32 whsize,
    float factor,
    int32 addborder,
    PIX ** ppixm,
    PIX ** ppixsd,
    PIX ** ppixth,
    PIX ** ppixd);
LEPT_DLL extern PIX * pixSauvolaOnContrastNorm(PIX * pixs, int32 mindiff, PIX ** ppixn, PIX ** ppixth);
LEPT_DLL extern PIX * pixThreshOnDoubleNorm(PIX * pixs, int32 mindiff);
LEPT_DLL extern l_ok pixThresholdByConnComp(PIX * pixs,
    PIX * pixm,
    int32 start,
    int32 end,
    int32 incr,
    float thresh48,
    float threshdiff,
    int32 * pglobthresh,
    PIX ** ppixd,
    int32 debugflag);
LEPT_DLL extern l_ok pixThresholdByHisto(PIX * pixs,
    int32 factor,
    int32 halfw,
    float delta,
    int32 * pthresh,
    PIX ** ppixd,
    PIX ** ppixhisto);
LEPT_DLL extern PIX * pixExpandBinaryReplicate(PIX * pixs, int32 xfact, int32 yfact);
LEPT_DLL extern PIX * pixExpandBinaryPower2(PIX * pixs, int32 factor);
LEPT_DLL extern PIX * pixReduceBinary2(PIX * pixs, uint8 * intab);
LEPT_DLL extern PIX * pixReduceRankBinaryCascade(PIX * pixs, int32 level1, int32 level2, int32 level3, int32 level4);
LEPT_DLL extern PIX * pixReduceRankBinary2(PIX * pixs, int32 level, uint8 * intab);
LEPT_DLL extern uint8 * makeSubsampleTab2x(void);
LEPT_DLL extern PIX * pixBlend(PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract);
LEPT_DLL extern PIX * pixBlendMask(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract, int32 type);
LEPT_DLL extern PIX * pixBlendGray(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract, int32 type, int32 transparent, uint32 transpix);
LEPT_DLL extern PIX * pixBlendGrayInverse(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract);
LEPT_DLL extern PIX * pixBlendColor(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract, int32 transparent, uint32 transpix);
LEPT_DLL extern PIX * pixBlendColorByChannel(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float rfract, float gfract, float bfract, int32 transparent, uint32 transpix);
LEPT_DLL extern PIX * pixBlendGrayAdapt(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract, int32 shift);
LEPT_DLL extern PIX * pixFadeWithGray(PIX * pixs, PIX * pixb, float factor, int32 type);
LEPT_DLL extern PIX * pixBlendHardLight(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 x, int32 y, float fract);
LEPT_DLL extern l_ok pixBlendCmap(PIX * pixs, PIX * pixb, int32 x, int32 y, int32 sindex);
LEPT_DLL extern PIX * pixBlendWithGrayMask(PIX * pixs1, PIX * pixs2, PIX * pixg, int32 x, int32 y);
LEPT_DLL extern PIX * pixBlendBackgroundToColor(PIX * pixd, PIX * pixs, BOX * box, uint32 color, float gamma, int32 minval, int32 maxval);
LEPT_DLL extern PIX * pixMultiplyByColor(PIX * pixd, PIX * pixs, BOX * box, uint32 color);
LEPT_DLL extern PIX * pixAlphaBlendUniform(PIX * pixs, uint32 color);
LEPT_DLL extern PIX * pixAddAlphaToBlend(PIX * pixs, float fract, int32 invert);
LEPT_DLL extern PIX * pixSetAlphaOverWhite(PIX * pixs);
LEPT_DLL extern l_ok pixLinearEdgeFade(PIX * pixs, int32 dir, int32 fadeto, float distfract, float maxfade);
LEPT_DLL extern L_BMF * bmfCreate(const char * dir, int32 fontsize);
LEPT_DLL extern void bmfDestroy(L_BMF ** pbmf);
LEPT_DLL extern PIX * bmfGetPix(L_BMF * bmf, char chr);
LEPT_DLL extern l_ok bmfGetWidth(L_BMF * bmf, char chr, int32 * pw);
LEPT_DLL extern l_ok bmfGetBaseline(L_BMF * bmf, char chr, int32 * pbaseline);
LEPT_DLL extern PIXA * pixaGetFont(const char * dir, int32 fontsize, int32 * pbl0, int32 * pbl1, int32 * pbl2);
LEPT_DLL extern l_ok pixaSaveFont(const char * indir, const char * outdir, int32 fontsize);
LEPT_DLL extern PIX * pixReadStreamBmp(FILE * fp);
LEPT_DLL extern PIX * pixReadMemBmp(const uint8 * cdata, size_t size);
LEPT_DLL extern l_ok pixWriteStreamBmp(FILE * fp, PIX * pix);
LEPT_DLL extern l_ok pixWriteMemBmp(uint8 ** pfdata, size_t * pfsize, PIX * pixs);
LEPT_DLL extern PIXA * l_bootnum_gen1(void);
LEPT_DLL extern PIXA * l_bootnum_gen2(void);
LEPT_DLL extern PIXA * l_bootnum_gen3(void);
LEPT_DLL extern PIXA * l_bootnum_gen4(int32 nsamp);
LEPT_DLL extern BOX * boxCreate(int32 x, int32 y, int32 w, int32 h);
LEPT_DLL extern BOX * boxCreateValid(int32 x, int32 y, int32 w, int32 h);
LEPT_DLL extern BOX * boxCopy(BOX * box);
LEPT_DLL extern BOX * boxClone(BOX * box);
LEPT_DLL extern void boxDestroy(BOX ** pbox);
LEPT_DLL extern l_ok boxGetGeometry(BOX * box, int32 * px, int32 * py, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok boxSetGeometry(BOX * box, int32 x, int32 y, int32 w, int32 h);
LEPT_DLL extern l_ok boxGetSideLocations(BOX * box, int32 * pl, int32 * pr, int32 * pt, int32 * pb);
LEPT_DLL extern l_ok boxSetSideLocations(BOX * box, int32 l, int32 r, int32 t, int32 b);
LEPT_DLL extern int32 boxGetRefcount(BOX * box);
LEPT_DLL extern l_ok boxChangeRefcount(BOX * box, int32 delta);
LEPT_DLL extern l_ok boxIsValid(BOX * box, int32 * pvalid);
LEPT_DLL extern BOXA * boxaCreate(int32 n);
LEPT_DLL extern BOXA * boxaCopy(BOXA * boxa, int32 copyflag);
LEPT_DLL extern void boxaDestroy(BOXA ** pboxa);
LEPT_DLL extern l_ok boxaAddBox(BOXA * boxa, BOX * box, int32 copyflag);
LEPT_DLL extern l_ok boxaExtendArray(BOXA * boxa);
LEPT_DLL extern l_ok boxaExtendArrayToSize(BOXA * boxa, size_t size);
LEPT_DLL extern int32 boxaGetCount(BOXA * boxa);
LEPT_DLL extern int32 boxaGetValidCount(BOXA * boxa);
LEPT_DLL extern BOX * boxaGetBox(BOXA * boxa, int32 index, int32 accessflag);
LEPT_DLL extern BOX * boxaGetValidBox(BOXA * boxa, int32 index, int32 accessflag);
LEPT_DLL extern NUMA * boxaFindInvalidBoxes(BOXA * boxa);
LEPT_DLL extern l_ok boxaGetBoxGeometry(BOXA * boxa, int32 index, int32 * px, int32 * py, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok boxaIsFull(BOXA * boxa, int32 * pfull);
LEPT_DLL extern l_ok boxaReplaceBox(BOXA * boxa, int32 index, BOX * box);
LEPT_DLL extern l_ok boxaInsertBox(BOXA * boxa, int32 index, BOX * box);
LEPT_DLL extern l_ok boxaRemoveBox(BOXA * boxa, int32 index);
LEPT_DLL extern l_ok boxaRemoveBoxAndSave(BOXA * boxa, int32 index, BOX ** pbox);
LEPT_DLL extern BOXA * boxaSaveValid(BOXA * boxas, int32 copyflag);
LEPT_DLL extern l_ok boxaInitFull(BOXA * boxa, BOX * box);
LEPT_DLL extern l_ok boxaClear(BOXA * boxa);
LEPT_DLL extern BOXAA * boxaaCreate(int32 n);
LEPT_DLL extern BOXAA * boxaaCopy(BOXAA * baas, int32 copyflag);
LEPT_DLL extern void boxaaDestroy(BOXAA ** pbaa);
LEPT_DLL extern l_ok boxaaAddBoxa(BOXAA * baa, BOXA * ba, int32 copyflag);
LEPT_DLL extern l_ok boxaaExtendArray(BOXAA * baa);
LEPT_DLL extern l_ok boxaaExtendArrayToSize(BOXAA * baa, int32 size);
LEPT_DLL extern int32 boxaaGetCount(BOXAA * baa);
LEPT_DLL extern int32 boxaaGetBoxCount(BOXAA * baa);
LEPT_DLL extern BOXA * boxaaGetBoxa(BOXAA * baa, int32 index, int32 accessflag);
LEPT_DLL extern BOX * boxaaGetBox(BOXAA * baa, int32 iboxa, int32 ibox, int32 accessflag);
LEPT_DLL extern l_ok boxaaInitFull(BOXAA * baa, BOXA * boxa);
LEPT_DLL extern l_ok boxaaExtendWithInit(BOXAA * baa, int32 maxindex, BOXA * boxa);
LEPT_DLL extern l_ok boxaaReplaceBoxa(BOXAA * baa, int32 index, BOXA * boxa);
LEPT_DLL extern l_ok boxaaInsertBoxa(BOXAA * baa, int32 index, BOXA * boxa);
LEPT_DLL extern l_ok boxaaRemoveBoxa(BOXAA * baa, int32 index);
LEPT_DLL extern l_ok boxaaAddBox(BOXAA * baa, int32 index, BOX * box, int32 accessflag);
LEPT_DLL extern BOXAA * boxaaReadFromFiles(const char * dirname, const char * substr, int32 first, int32 nfiles);
LEPT_DLL extern BOXAA * boxaaRead(const char * filename);
LEPT_DLL extern BOXAA * boxaaReadStream(FILE * fp);
LEPT_DLL extern BOXAA * boxaaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok boxaaWrite(const char * filename, BOXAA * baa);
LEPT_DLL extern l_ok boxaaWriteStream(FILE * fp, BOXAA * baa);
LEPT_DLL extern l_ok boxaaWriteMem(uint8 ** pdata, size_t * psize, BOXAA * baa);
LEPT_DLL extern BOXA * boxaRead(const char * filename);
LEPT_DLL extern BOXA * boxaReadStream(FILE * fp);
LEPT_DLL extern BOXA * boxaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok boxaWriteDebug(const char * filename, BOXA * boxa);
LEPT_DLL extern l_ok boxaWrite(const char * filename, BOXA * boxa);
LEPT_DLL extern l_ok boxaWriteStream(FILE * fp, BOXA * boxa);
LEPT_DLL extern l_ok boxaWriteStderr(BOXA * boxa);
LEPT_DLL extern l_ok boxaWriteMem(uint8 ** pdata, size_t * psize, BOXA * boxa);
LEPT_DLL extern l_ok boxPrintStreamInfo(FILE * fp, BOX * box);
LEPT_DLL extern l_ok boxContains(BOX * box1, BOX * box2, int32 * presult);
LEPT_DLL extern l_ok boxIntersects(BOX * box1, BOX * box2, int32 * presult);
LEPT_DLL extern BOXA * boxaContainedInBox(BOXA * boxas, BOX * box);
LEPT_DLL extern l_ok boxaContainedInBoxCount(BOXA * boxa, BOX * box, int32 * pcount);
LEPT_DLL extern l_ok boxaContainedInBoxa(BOXA * boxa1, BOXA * boxa2, int32 * pcontained);
LEPT_DLL extern BOXA * boxaIntersectsBox(BOXA * boxas, BOX * box);
LEPT_DLL extern l_ok boxaIntersectsBoxCount(BOXA * boxa, BOX * box, int32 * pcount);
LEPT_DLL extern BOXA * boxaClipToBox(BOXA * boxas, BOX * box);
LEPT_DLL extern BOXA * boxaCombineOverlaps(BOXA * boxas, PIXA * pixadb);
LEPT_DLL extern l_ok boxaCombineOverlapsInPair(BOXA * boxas1, BOXA * boxas2, BOXA ** pboxad1, BOXA ** pboxad2, PIXA * pixadb);
LEPT_DLL extern BOX * boxOverlapRegion(BOX * box1, BOX * box2);
LEPT_DLL extern BOX * boxBoundingRegion(BOX * box1, BOX * box2);
LEPT_DLL extern l_ok boxOverlapFraction(BOX * box1, BOX * box2, float * pfract);
LEPT_DLL extern l_ok boxOverlapArea(BOX * box1, BOX * box2, int32 * parea);
LEPT_DLL extern BOXA * boxaHandleOverlaps(BOXA * boxas,
    int32 op,
    int32 range,
    float min_overlap,
    float max_ratio,
    NUMA ** pnamap);
LEPT_DLL extern l_ok boxOverlapDistance(BOX * box1, BOX * box2, int32 * ph_ovl, int32 * pv_ovl);
LEPT_DLL extern l_ok boxSeparationDistance(BOX * box1, BOX * box2, int32 * ph_sep, int32 * pv_sep);
LEPT_DLL extern l_ok boxCompareSize(BOX * box1, BOX * box2, int32 type, int32 * prel);
LEPT_DLL extern l_ok boxContainsPt(BOX * box, float x, float y, int32 * pcontains);
LEPT_DLL extern BOX * boxaGetNearestToPt(BOXA * boxa, int32 x, int32 y);
LEPT_DLL extern BOX * boxaGetNearestToLine(BOXA * boxa, int32 x, int32 y);
LEPT_DLL extern l_ok boxaFindNearestBoxes(BOXA * boxa, int32 dist_select, int32 range, NUMAA ** pnaaindex, NUMAA ** pnaadist);
LEPT_DLL extern l_ok boxaGetNearestByDirection(BOXA * boxa,
    int32 i,
    int32 dir,
    int32 dist_select,
    int32 range,
    int32 * pindex,
    int32 * pdist);
LEPT_DLL extern l_ok boxGetCenter(BOX * box, float * pcx, float * pcy);
LEPT_DLL extern l_ok boxIntersectByLine(BOX * box,
    int32 x,
    int32 y,
    float slope,
    int32 * px1,
    int32 * py1,
    int32 * px2,
    int32 * py2,
    int32 * pn);
LEPT_DLL extern BOX * boxClipToRectangle(BOX * box, int32 wi, int32 hi);
LEPT_DLL extern l_ok boxClipToRectangleParams(BOX * box,
    int32 w,
    int32 h,
    int32 * pxstart,
    int32 * pystart,
    int32 * pxend,
    int32 * pyend,
    int32 * pbw,
    int32 * pbh);
LEPT_DLL extern BOX * boxRelocateOneSide(BOX * boxd, BOX * boxs, int32 loc, int32 sideflag);
LEPT_DLL extern BOXA * boxaAdjustSides(BOXA * boxas, int32 delleft, int32 delright, int32 deltop, int32 delbot);
LEPT_DLL extern l_ok boxaAdjustBoxSides(BOXA * boxa, int32 index, int32 delleft, int32 delright, int32 deltop, int32 delbot);
LEPT_DLL extern BOX * boxAdjustSides(BOX * boxd, BOX * boxs, int32 delleft, int32 delright, int32 deltop, int32 delbot);
LEPT_DLL extern BOXA * boxaSetSide(BOXA * boxad, BOXA * boxas, int32 side, int32 val, int32 thresh);
LEPT_DLL extern l_ok boxSetSide(BOX * boxs, int32 side, int32 val, int32 thresh);
LEPT_DLL extern BOXA * boxaAdjustWidthToTarget(BOXA * boxad, BOXA * boxas, int32 sides, int32 target, int32 thresh);
LEPT_DLL extern BOXA * boxaAdjustHeightToTarget(BOXA * boxad, BOXA * boxas, int32 sides, int32 target, int32 thresh);
LEPT_DLL extern l_ok boxEqual(BOX * box1, BOX * box2, int32 * psame);
LEPT_DLL extern l_ok boxaEqual(BOXA * boxa1, BOXA * boxa2, int32 maxdist, NUMA ** pnaindex, int32 * psame);
LEPT_DLL extern l_ok boxSimilar(BOX * box1,
    BOX * box2,
    int32 leftdiff,
    int32 rightdiff,
    int32 topdiff,
    int32 botdiff,
    int32 * psimilar);
LEPT_DLL extern l_ok boxaSimilar(BOXA * boxa1,
    BOXA * boxa2,
    int32 leftdiff,
    int32 rightdiff,
    int32 topdiff,
    int32 botdiff,
    int32 debug,
    int32 * psimilar,
    NUMA ** pnasim);
LEPT_DLL extern l_ok boxaJoin(BOXA * boxad, BOXA * boxas, int32 istart, int32 iend);
LEPT_DLL extern l_ok boxaaJoin(BOXAA * baad, BOXAA * baas, int32 istart, int32 iend);
LEPT_DLL extern l_ok boxaSplitEvenOdd(BOXA * boxa, int32 fillflag, BOXA ** pboxae, BOXA ** pboxao);
LEPT_DLL extern BOXA * boxaMergeEvenOdd(BOXA * boxae, BOXA * boxao, int32 fillflag);
LEPT_DLL extern BOXA * boxaTransform(BOXA * boxas, int32 shiftx, int32 shifty, float scalex, float scaley);
LEPT_DLL extern BOX * boxTransform(BOX * box, int32 shiftx, int32 shifty, float scalex, float scaley);
LEPT_DLL extern BOXA * boxaTransformOrdered(BOXA * boxas,
    int32 shiftx,
    int32 shifty,
    float scalex,
    float scaley,
    int32 xcen,
    int32 ycen,
    float angle,
    int32 order);
LEPT_DLL extern BOX * boxTransformOrdered(BOX * boxs,
    int32 shiftx,
    int32 shifty,
    float scalex,
    float scaley,
    int32 xcen,
    int32 ycen,
    float angle,
    int32 order);
LEPT_DLL extern BOXA * boxaRotateOrth(BOXA * boxas, int32 w, int32 h, int32 rotation);
LEPT_DLL extern BOX * boxRotateOrth(BOX * box, int32 w, int32 h, int32 rotation);
LEPT_DLL extern BOXA * boxaShiftWithPta(BOXA * boxas, PTA * pta, int32 dir);
LEPT_DLL extern BOXA * boxaSort(BOXA * boxas, int32 sorttype, int32 sortorder, NUMA ** pnaindex);
LEPT_DLL extern BOXA * boxaBinSort(BOXA * boxas, int32 sorttype, int32 sortorder, NUMA ** pnaindex);
LEPT_DLL extern BOXA * boxaSortByIndex(BOXA * boxas, NUMA * naindex);
LEPT_DLL extern BOXAA * boxaSort2d(BOXA * boxas, NUMAA ** pnaad, int32 delta1, int32 delta2, int32 minh1);
LEPT_DLL extern BOXAA * boxaSort2dByIndex(BOXA * boxas, NUMAA * naa);
LEPT_DLL extern l_ok boxaExtractAsNuma(BOXA * boxa,
    NUMA ** pnal,
    NUMA ** pnat,
    NUMA ** pnar,
    NUMA ** pnab,
    NUMA ** pnaw,
    NUMA ** pnah,
    int32 keepinvalid);
LEPT_DLL extern l_ok boxaExtractAsPta(BOXA * boxa,
    PTA ** pptal,
    PTA ** pptat,
    PTA ** pptar,
    PTA ** pptab,
    PTA ** pptaw,
    PTA ** pptah,
    int32 keepinvalid);
LEPT_DLL extern PTA * boxaExtractCorners(BOXA * boxa, int32 loc);
LEPT_DLL extern l_ok boxaGetRankVals(BOXA * boxa,
    float fract,
    int32 * px,
    int32 * py,
    int32 * pr,
    int32 * pb,
    int32 * pw,
    int32 * ph);
LEPT_DLL extern l_ok boxaGetMedianVals(BOXA * boxa, int32 * px, int32 * py, int32 * pr, int32 * pb, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok boxaGetAverageSize(BOXA * boxa, float * pw, float * ph);
LEPT_DLL extern l_ok boxaaGetExtent(BOXAA * baa, int32 * pw, int32 * ph, BOX ** pbox, BOXA ** pboxa);
LEPT_DLL extern BOXA * boxaaFlattenToBoxa(BOXAA * baa, NUMA ** pnaindex, int32 copyflag);
LEPT_DLL extern BOXA * boxaaFlattenAligned(BOXAA * baa, int32 num, BOX * fillerbox, int32 copyflag);
LEPT_DLL extern BOXAA * boxaEncapsulateAligned(BOXA * boxa, int32 num, int32 copyflag);
LEPT_DLL extern BOXAA * boxaaTranspose(BOXAA * baas);
LEPT_DLL extern l_ok boxaaAlignBox(BOXAA * baa, BOX * box, int32 delta, int32 * pindex);
LEPT_DLL extern PIX * pixMaskConnComp(PIX * pixs, int32 connectivity, BOXA ** pboxa);
LEPT_DLL extern PIX * pixMaskBoxa(PIX * pixd, PIX * pixs, BOXA * boxa, int32 op);
LEPT_DLL extern PIX * pixPaintBoxa(PIX * pixs, BOXA * boxa, uint32 val);
LEPT_DLL extern PIX * pixSetBlackOrWhiteBoxa(PIX * pixs, BOXA * boxa, int32 op);
LEPT_DLL extern PIX * pixPaintBoxaRandom(PIX * pixs, BOXA * boxa);
LEPT_DLL extern PIX * pixBlendBoxaRandom(PIX * pixs, BOXA * boxa, float fract);
LEPT_DLL extern PIX * pixDrawBoxa(PIX * pixs, BOXA * boxa, int32 width, uint32 val);
LEPT_DLL extern PIX * pixDrawBoxaRandom(PIX * pixs, BOXA * boxa, int32 width);
LEPT_DLL extern PIX * boxaaDisplay(PIX * pixs,
    BOXAA * baa,
    int32 linewba,
    int32 linewb,
    uint32 colorba,
    uint32 colorb,
    int32 w,
    int32 h);
LEPT_DLL extern PIXA * pixaDisplayBoxaa(PIXA * pixas, BOXAA * baa, int32 colorflag, int32 width);
LEPT_DLL extern BOXA * pixSplitIntoBoxa(PIX * pixs, int32 minsum, int32 skipdist, int32 delta, int32 maxbg, int32 maxcomps, int32 remainder);
LEPT_DLL extern BOXA * pixSplitComponentIntoBoxa(PIX * pix, BOX * box, int32 minsum, int32 skipdist, int32 delta, int32 maxbg, int32 maxcomps, int32 remainder);
LEPT_DLL extern BOXA * makeMosaicStrips(int32 w, int32 h, int32 direction, int32 size);
LEPT_DLL extern l_ok boxaCompareRegions(BOXA * boxa1, BOXA * boxa2, int32 areathresh, int32 * pnsame, float * pdiffarea, float * pdiffxor, PIX ** ppixdb);
LEPT_DLL extern BOX * pixSelectLargeULComp(PIX * pixs, float areaslop, int32 yslop, int32 connectivity);
LEPT_DLL extern BOX * boxaSelectLargeULBox(BOXA * boxas, float areaslop, int32 yslop);
LEPT_DLL extern BOXA * boxaSelectRange(BOXA * boxas, int32 first, int32 last, int32 copyflag);
LEPT_DLL extern BOXAA * boxaaSelectRange(BOXAA * baas, int32 first, int32 last, int32 copyflag);
LEPT_DLL extern BOXA * boxaSelectBySize(BOXA * boxas, int32 width, int32 height, int32 type, int32 relation, int32 * pchanged);
LEPT_DLL extern NUMA * boxaMakeSizeIndicator(BOXA * boxa, int32 width, int32 height, int32 type, int32 relation);
LEPT_DLL extern BOXA * boxaSelectByArea(BOXA * boxas, int32 area, int32 relation, int32 * pchanged);
LEPT_DLL extern NUMA * boxaMakeAreaIndicator(BOXA * boxa, int32 area, int32 relation);
LEPT_DLL extern BOXA * boxaSelectByWHRatio(BOXA * boxas, float ratio, int32 relation, int32 * pchanged);
LEPT_DLL extern NUMA * boxaMakeWHRatioIndicator(BOXA * boxa, float ratio, int32 relation);
LEPT_DLL extern BOXA * boxaSelectWithIndicator(BOXA * boxas, NUMA * na, int32 * pchanged);
LEPT_DLL extern BOXA * boxaPermutePseudorandom(BOXA * boxas);
LEPT_DLL extern BOXA * boxaPermuteRandom(BOXA * boxad, BOXA * boxas);
LEPT_DLL extern l_ok boxaSwapBoxes(BOXA * boxa, int32 i, int32 j);
LEPT_DLL extern PTA * boxaConvertToPta(BOXA * boxa, int32 ncorners);
LEPT_DLL extern BOXA * ptaConvertToBoxa(PTA * pta, int32 ncorners);
LEPT_DLL extern PTA * boxConvertToPta(BOX * box, int32 ncorners);
LEPT_DLL extern BOX * ptaConvertToBox(PTA * pta);
LEPT_DLL extern l_ok boxaGetExtent(BOXA * boxa, int32 * pw, int32 * ph, BOX ** pbox);
LEPT_DLL extern l_ok boxaGetCoverage(BOXA * boxa, int32 wc, int32 hc, int32 exactflag, float * pfract);
LEPT_DLL extern l_ok boxaaSizeRange(BOXAA * baa, int32 * pminw, int32 * pminh, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern l_ok boxaSizeRange(BOXA * boxa, int32 * pminw, int32 * pminh, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern l_ok boxaLocationRange(BOXA * boxa, int32 * pminx, int32 * pminy, int32 * pmaxx, int32 * pmaxy);
LEPT_DLL extern l_ok boxaGetSizes(BOXA * boxa, NUMA ** pnaw, NUMA ** pnah);
LEPT_DLL extern l_ok boxaGetArea(BOXA * boxa, int32 * parea);
LEPT_DLL extern PIX * boxaDisplayTiled(BOXA * boxas,
    PIXA * pixa,
    int32 first,
    int32 last,
    int32 maxwidth,
    int32 linewidth,
    float scalefactor,
    int32 background,
    int32 spacing,
    int32 border);
LEPT_DLL extern BOXA * boxaSmoothSequenceMedian(BOXA * boxas,
    int32 halfwin,
    int32 subflag,
    int32 maxdiff,
    int32 extrapixels,
    int32 debug);
LEPT_DLL extern BOXA * boxaWindowedMedian(BOXA * boxas, int32 halfwin, int32 debug);
LEPT_DLL extern BOXA * boxaModifyWithBoxa(BOXA * boxas, BOXA * boxam, int32 subflag, int32 maxdiff, int32 extrapixels);
LEPT_DLL extern BOXA * boxaReconcilePairWidth(BOXA * boxas, int32 delw, int32 op, float factor, NUMA * na);
LEPT_DLL extern l_ok boxaSizeConsistency(BOXA * boxas,
    int32 type,
    float threshp,
    float threshm,
    float * pfvarp,
    float * pfvarm,
    int32 * psame);
LEPT_DLL extern BOXA * boxaReconcileAllByMedian(BOXA * boxas, int32 select1, int32 select2, int32 thresh, int32 extra,
    PIXA * pixadb);
LEPT_DLL extern BOXA * boxaReconcileSidesByMedian(BOXA * boxas, int32 select, int32 thresh, int32 extra, PIXA * pixadb);
LEPT_DLL extern BOXA * boxaReconcileSizeByMedian(BOXA * boxas,
    int32 type,
    float dfract,
    float sfract,
    float factor,
    NUMA ** pnadelw,
    NUMA ** pnadelh,
    float * pratiowh);
LEPT_DLL extern l_ok boxaPlotSides(BOXA * boxa, const char * plotname, NUMA ** pnal, NUMA ** pnat, NUMA ** pnar, NUMA ** pnab,
    PIX ** ppixd);
LEPT_DLL extern l_ok boxaPlotSizes(BOXA * boxa, const char * plotname, NUMA ** pnaw, NUMA ** pnah, PIX ** ppixd);
LEPT_DLL extern BOXA * boxaFillSequence(BOXA * boxas, int32 useflag, int32 debug);
LEPT_DLL extern l_ok boxaSizeVariation(BOXA * boxa,
    int32 type,
    float * pdel_evenodd,
    float * prms_even,
    float * prms_odd,
    float * prms_all);
LEPT_DLL extern l_ok boxaMedianDimensions(BOXA * boxas,
    int32 * pmedw,
    int32 * pmedh,
    int32 * pmedwe,
    int32 * pmedwo,
    int32 * pmedhe,
    int32 * pmedho,
    NUMA ** pnadelw,
    NUMA ** pnadelh);
LEPT_DLL extern L_BYTEA * l_byteaCreate(size_t nbytes);
LEPT_DLL extern L_BYTEA * l_byteaInitFromMem(const uint8 * data, size_t size);
LEPT_DLL extern L_BYTEA * l_byteaInitFromFile(const char * fname);
LEPT_DLL extern L_BYTEA * l_byteaInitFromStream(FILE * fp);
LEPT_DLL extern L_BYTEA * l_byteaCopy(L_BYTEA * bas, int32 copyflag);
LEPT_DLL extern void l_byteaDestroy(L_BYTEA ** pba);
LEPT_DLL extern size_t l_byteaGetSize(L_BYTEA * ba);
LEPT_DLL extern uint8 * l_byteaGetData(L_BYTEA * ba, size_t * psize);
LEPT_DLL extern uint8 * l_byteaCopyData(L_BYTEA * ba, size_t * psize);
LEPT_DLL extern l_ok l_byteaAppendData(L_BYTEA * ba, const uint8 * newdata, size_t newbytes);
LEPT_DLL extern l_ok l_byteaAppendString(L_BYTEA * ba, const char * str);
LEPT_DLL extern l_ok l_byteaJoin(L_BYTEA * ba1, L_BYTEA ** pba2);
LEPT_DLL extern l_ok l_byteaSplit(L_BYTEA * ba1, size_t splitloc, L_BYTEA ** pba2);
LEPT_DLL extern l_ok l_byteaFindEachSequence(L_BYTEA * ba, const uint8 * sequence, size_t seqlen, L_DNA ** pda);
LEPT_DLL extern l_ok l_byteaWrite(const char * fname, L_BYTEA * ba, size_t startloc, size_t nbytes);
LEPT_DLL extern l_ok l_byteaWriteStream(FILE * fp, L_BYTEA * ba, size_t startloc, size_t nbytes);
LEPT_DLL extern CCBORDA * ccbaCreate(PIX * pixs, int32 n);
LEPT_DLL extern void ccbaDestroy(CCBORDA ** pccba);
LEPT_DLL extern CCBORD * ccbCreate(PIX * pixs);
LEPT_DLL extern void ccbDestroy(CCBORD ** pccb);
LEPT_DLL extern l_ok ccbaAddCcb(CCBORDA * ccba, CCBORD * ccb);
LEPT_DLL extern int32 ccbaGetCount(CCBORDA * ccba);
LEPT_DLL extern CCBORD * ccbaGetCcb(CCBORDA * ccba, int32 index);
LEPT_DLL extern CCBORDA * pixGetAllCCBorders(PIX * pixs);
LEPT_DLL extern PTAA * pixGetOuterBordersPtaa(PIX * pixs);
LEPT_DLL extern l_ok pixGetOuterBorder(CCBORD * ccb, PIX * pixs, BOX * box);
LEPT_DLL extern l_ok ccbaGenerateGlobalLocs(CCBORDA * ccba);
LEPT_DLL extern l_ok ccbaGenerateStepChains(CCBORDA * ccba);
LEPT_DLL extern l_ok ccbaStepChainsToPixCoords(CCBORDA * ccba, int32 coordtype);
LEPT_DLL extern l_ok ccbaGenerateSPGlobalLocs(CCBORDA * ccba, int32 ptsflag);
LEPT_DLL extern l_ok ccbaGenerateSinglePath(CCBORDA * ccba);
LEPT_DLL extern PTA * getCutPathForHole(PIX * pix, PTA * pta, BOX * boxinner, int32 * pdir, int32 * plen);
LEPT_DLL extern PIX * ccbaDisplayBorder(CCBORDA * ccba);
LEPT_DLL extern PIX * ccbaDisplaySPBorder(CCBORDA * ccba);
LEPT_DLL extern PIX * ccbaDisplayImage1(CCBORDA * ccba);
LEPT_DLL extern PIX * ccbaDisplayImage2(CCBORDA * ccba);
LEPT_DLL extern l_ok ccbaWrite(const char * filename, CCBORDA * ccba);
LEPT_DLL extern l_ok ccbaWriteStream(FILE * fp, CCBORDA * ccba);
LEPT_DLL extern CCBORDA * ccbaRead(const char * filename);
LEPT_DLL extern CCBORDA * ccbaReadStream(FILE * fp);
LEPT_DLL extern l_ok ccbaWriteSVG(const char * filename, CCBORDA * ccba);
LEPT_DLL extern char * ccbaWriteSVGString(CCBORDA * ccba);
LEPT_DLL extern PIXA * pixaThinConnected(PIXA * pixas, int32 type, int32 connectivity, int32 maxiters);
LEPT_DLL extern PIX * pixThinConnected(PIX * pixs, int32 type, int32 connectivity, int32 maxiters);
LEPT_DLL extern PIX * pixThinConnectedBySet(PIX * pixs, int32 type, SELA * sela, int32 maxiters);
LEPT_DLL extern SELA * selaMakeThinSets(int32 index, int32 debug);
LEPT_DLL extern l_ok pixFindCheckerboardCorners(PIX * pixs,
    int32 size,
    int32 dilation,
    int32 nsels,
    PIX ** ppix_corners,
    PTA ** ppta_corners,
    PIXA * pixadb);
LEPT_DLL extern l_ok jbCorrelation(const char * dirin,
    float thresh,
    float weight,
    int32 components,
    const char * rootname,
    int32 firstpage,
    int32 npages,
    int32 renderflag);
LEPT_DLL extern l_ok jbRankHaus(const char * dirin,
    int32 size,
    float rank,
    int32 components,
    const char * rootname,
    int32 firstpage,
    int32 npages,
    int32 renderflag);
LEPT_DLL extern JBCLASSER * jbWordsInTextlines(const char * dirin,
    int32 reduction,
    int32 maxwidth,
    int32 maxheight,
    float thresh,
    float weight,
    NUMA ** pnatl,
    int32 firstpage,
    int32 npages);
LEPT_DLL extern l_ok pixGetWordsInTextlines(PIX * pixs,
    int32 minwidth,
    int32 minheight,
    int32 maxwidth,
    int32 maxheight,
    BOXA ** pboxad,
    PIXA ** ppixad,
    NUMA ** pnai);
LEPT_DLL extern l_ok pixGetWordBoxesInTextlines(PIX * pixs,
    int32 minwidth,
    int32 minheight,
    int32 maxwidth,
    int32 maxheight,
    BOXA ** pboxad,
    NUMA ** pnai);
LEPT_DLL extern l_ok pixFindWordAndCharacterBoxes(PIX * pixs,
    BOX * boxs,
    int32 thresh,
    BOXA ** pboxaw,
    BOXAA ** pboxaac,
    const char * debugdir);
LEPT_DLL extern NUMAA * boxaExtractSortedPattern(BOXA * boxa, NUMA * na);
LEPT_DLL extern l_ok numaaCompareImagesByBoxes(NUMAA * naa1,
    NUMAA * naa2,
    int32 nperline,
    int32 nreq,
    int32 maxshiftx,
    int32 maxshifty,
    int32 delx,
    int32 dely,
    int32 * psame,
    int32 debugflag);
LEPT_DLL extern l_ok pixColorContent(PIX * pixs,
    int32 rref,
    int32 gref,
    int32 bref,
    int32 mingray,
    PIX ** ppixr,
    PIX ** ppixg,
    PIX ** ppixb);
LEPT_DLL extern PIX * pixColorMagnitude(PIX * pixs, int32 rref, int32 gref, int32 bref, int32 type);
LEPT_DLL extern l_ok pixColorFraction(PIX * pixs,
    int32 darkthresh,
    int32 lightthresh,
    int32 diffthresh,
    int32 factor,
    float * ppixfract,
    float * pcolorfract);
LEPT_DLL extern PIX * pixColorShiftWhitePoint(PIX * pixs, int32 rref, int32 gref, int32 bref);
LEPT_DLL extern PIX * pixMaskOverColorPixels(PIX * pixs, int32 threshdiff, int32 mindist);
LEPT_DLL extern PIX * pixMaskOverGrayPixels(PIX * pixs, int32 maxlimit, int32 satlimit);
LEPT_DLL extern PIX * pixMaskOverColorRange(PIX * pixs, int32 rmin, int32 rmax, int32 gmin, int32 gmax, int32 bmin, int32 bmax);
LEPT_DLL extern l_ok pixFindColorRegions(PIX * pixs,
    PIX * pixm,
    int32 factor,
    int32 lightthresh,
    int32 darkthresh,
    int32 mindiff,
    int32 colordiff,
    float edgefract,
    float * pcolorfract,
    PIX ** pcolormask1,
    PIX ** pcolormask2,
    PIXA * pixadb);
LEPT_DLL extern l_ok pixNumSignificantGrayColors(PIX * pixs,
    int32 darkthresh,
    int32 lightthresh,
    float minfract,
    int32 factor,
    int32 * pncolors);
LEPT_DLL extern l_ok pixColorsForQuantization(PIX * pixs, int32 thresh, int32 * pncolors, int32 * piscolor, int32 debug);
LEPT_DLL extern l_ok pixNumColors(PIX * pixs, int32 factor, int32 * pncolors);
LEPT_DLL extern PIX * pixConvertRGBToCmapLossless(PIX * pixs);
LEPT_DLL extern l_ok pixGetMostPopulatedColors(PIX * pixs,
    int32 sigbits,
    int32 factor,
    int32 ncolors,
    uint32 ** parray,
    PIXCMAP ** pcmap);
LEPT_DLL extern PIX * pixSimpleColorQuantize(PIX * pixs, int32 sigbits, int32 factor, int32 ncolors);
LEPT_DLL extern NUMA * pixGetRGBHistogram(PIX * pixs, int32 sigbits, int32 factor);
LEPT_DLL extern l_ok makeRGBIndexTables(uint32 ** prtab, uint32 ** pgtab, uint32 ** pbtab, int32 sigbits);
LEPT_DLL extern l_ok getRGBFromIndex(uint32 index, int32 sigbits, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern l_ok pixHasHighlightRed(PIX * pixs,
    int32 factor,
    float minfract,
    float fthresh,
    int32 * phasred,
    float * pratio,
    PIX ** ppixdb);
LEPT_DLL extern L_COLORFILL * l_colorfillCreate(PIX * pixs, int32 nx, int32 ny);
LEPT_DLL extern void l_colorfillDestroy(L_COLORFILL ** pcf);
LEPT_DLL extern l_ok pixColorContentByLocation(L_COLORFILL * cf,
    int32 rref,
    int32 gref,
    int32 bref,
    int32 minmax,
    int32 maxdiff,
    int32 minarea,
    int32 smooth,
    int32 debug);
LEPT_DLL extern PIX * pixColorFill(PIX * pixs, int32 minmax, int32 maxdiff, int32 smooth, int32 minarea, int32 debug);
LEPT_DLL extern PIXA * makeColorfillTestData(int32 w, int32 h, int32 nseeds, int32 range);
LEPT_DLL extern PIX * pixColorGrayRegions(PIX * pixs, BOXA * boxa, int32 type, int32 thresh, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixColorGray(PIX * pixs, BOX * box, int32 type, int32 thresh, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern PIX * pixColorGrayMasked(PIX * pixs, PIX * pixm, int32 type, int32 thresh, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern PIX * pixSnapColor(PIX * pixd, PIX * pixs, uint32 srcval, uint32 dstval, int32 diff);
LEPT_DLL extern PIX * pixSnapColorCmap(PIX * pixd, PIX * pixs, uint32 srcval, uint32 dstval, int32 diff);
LEPT_DLL extern PIX * pixLinearMapToTargetColor(PIX * pixd, PIX * pixs, uint32 srcval, uint32 dstval);
LEPT_DLL extern l_ok pixelLinearMapToTargetColor(uint32 scolor, uint32 srcmap, uint32 dstmap, uint32 * pdcolor);
LEPT_DLL extern PIX * pixShiftByComponent(PIX * pixd, PIX * pixs, uint32 srcval, uint32 dstval);
LEPT_DLL extern l_ok pixelShiftByComponent(int32 rval, int32 gval, int32 bval, uint32 srcval, uint32 dstval, uint32 * ppixel);
LEPT_DLL extern l_ok pixelFractionalShift(int32 rval, int32 gval, int32 bval, float fract, uint32 * ppixel);
LEPT_DLL extern PIX * pixMapWithInvariantHue(PIX * pixd, PIX * pixs, uint32 srcval, float fract);
LEPT_DLL extern PIXCMAP * pixcmapCreate(int32 depth);
LEPT_DLL extern PIXCMAP * pixcmapCreateRandom(int32 depth, int32 hasblack, int32 haswhite);
LEPT_DLL extern PIXCMAP * pixcmapCreateLinear(int32 d, int32 nlevels);
LEPT_DLL extern PIXCMAP * pixcmapCopy(const PIXCMAP * cmaps);
LEPT_DLL extern void pixcmapDestroy(PIXCMAP ** pcmap);
LEPT_DLL extern l_ok pixcmapIsValid(const PIXCMAP * cmap, PIX * pix, int32 * pvalid);
LEPT_DLL extern l_ok pixcmapAddColor(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixcmapAddRGBA(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 aval);
LEPT_DLL extern l_ok pixcmapAddNewColor(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 * pindex);
LEPT_DLL extern l_ok pixcmapAddNearestColor(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 * pindex);
LEPT_DLL extern l_ok pixcmapUsableColor(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 * pusable);
LEPT_DLL extern l_ok pixcmapAddBlackOrWhite(PIXCMAP * cmap, int32 color, int32 * pindex);
LEPT_DLL extern l_ok pixcmapSetBlackAndWhite(PIXCMAP * cmap, int32 setblack, int32 setwhite);
LEPT_DLL extern int32 pixcmapGetCount(const PIXCMAP * cmap);
LEPT_DLL extern int32 pixcmapGetFreeCount(PIXCMAP * cmap);
LEPT_DLL extern int32 pixcmapGetDepth(PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapGetMinDepth(PIXCMAP * cmap, int32 * pmindepth);
LEPT_DLL extern l_ok pixcmapClear(PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapGetColor(PIXCMAP * cmap, int32 index, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern l_ok pixcmapGetColor32(PIXCMAP * cmap, int32 index, uint32 * pval32);
LEPT_DLL extern l_ok pixcmapGetRGBA(PIXCMAP * cmap, int32 index, int32 * prval, int32 * pgval, int32 * pbval, int32 * paval);
LEPT_DLL extern l_ok pixcmapGetRGBA32(PIXCMAP * cmap, int32 index, uint32 * pval32);
LEPT_DLL extern l_ok pixcmapResetColor(PIXCMAP * cmap, int32 index, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixcmapSetAlpha(PIXCMAP * cmap, int32 index, int32 aval);
LEPT_DLL extern int32 pixcmapGetIndex(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 * pindex);
LEPT_DLL extern l_ok pixcmapHasColor(PIXCMAP * cmap, int32 * pcolor);
LEPT_DLL extern l_ok pixcmapIsOpaque(PIXCMAP * cmap, int32 * popaque);
LEPT_DLL extern l_ok pixcmapNonOpaqueColorsInfo(PIXCMAP * cmap, int32 * pntrans, int32 * pmax_trans, int32 * pmin_opaque);
LEPT_DLL extern l_ok pixcmapIsBlackAndWhite(PIXCMAP * cmap, int32 * pblackwhite);
LEPT_DLL extern l_ok pixcmapCountGrayColors(PIXCMAP * cmap, int32 * pngray);
LEPT_DLL extern l_ok pixcmapGetRankIntensity(PIXCMAP * cmap, float rankval, int32 * pindex);
LEPT_DLL extern l_ok pixcmapGetNearestIndex(PIXCMAP * cmap, int32 rval, int32 gval, int32 bval, int32 * pindex);
LEPT_DLL extern l_ok pixcmapGetNearestGrayIndex(PIXCMAP * cmap, int32 val, int32 * pindex);
LEPT_DLL extern l_ok pixcmapGetDistanceToColor(PIXCMAP * cmap, int32 index, int32 rval, int32 gval, int32 bval, int32 * pdist);
LEPT_DLL extern l_ok pixcmapGetRangeValues(PIXCMAP * cmap,
    int32 select,
    int32 * pminval,
    int32 * pmaxval,
    int32 * pminindex,
    int32 * pmaxindex);
LEPT_DLL extern PIXCMAP * pixcmapGrayToFalseColor(float gamma);
LEPT_DLL extern PIXCMAP * pixcmapGrayToColor(uint32 color);
LEPT_DLL extern PIXCMAP * pixcmapColorToGray(PIXCMAP * cmaps, float rwt, float gwt, float bwt);
LEPT_DLL extern PIXCMAP * pixcmapConvertTo4(PIXCMAP * cmaps);
LEPT_DLL extern PIXCMAP * pixcmapConvertTo8(PIXCMAP * cmaps);
LEPT_DLL extern PIXCMAP * pixcmapRead(const char * filename);
LEPT_DLL extern PIXCMAP * pixcmapReadStream(FILE * fp);
LEPT_DLL extern PIXCMAP * pixcmapReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixcmapWrite(const char * filename, const PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapWriteStream(FILE * fp, const PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapWriteMem(uint8 ** pdata, size_t * psize, const PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapToArrays(const PIXCMAP * cmap, int32 ** prmap, int32 ** pgmap, int32 ** pbmap, int32 ** pamap);
LEPT_DLL extern l_ok pixcmapToRGBTable(PIXCMAP * cmap, uint32 ** ptab, int32 * pncolors);
LEPT_DLL extern l_ok pixcmapSerializeToMemory(PIXCMAP * cmap, int32 cpc, int32 * pncolors, uint8 ** pdata);
LEPT_DLL extern PIXCMAP * pixcmapDeserializeFromMemory(uint8 * data, int32 cpc, int32 ncolors);
LEPT_DLL extern char * pixcmapConvertToHex(uint8 * data, int32 ncolors);
LEPT_DLL extern l_ok pixcmapGammaTRC(PIXCMAP * cmap, float gamma, int32 minval, int32 maxval);
LEPT_DLL extern l_ok pixcmapContrastTRC(PIXCMAP * cmap, float factor);
LEPT_DLL extern l_ok pixcmapShiftIntensity(PIXCMAP * cmap, float fraction);
LEPT_DLL extern l_ok pixcmapShiftByComponent(PIXCMAP * cmap, uint32 srcval, uint32 dstval);
LEPT_DLL extern PIX * pixColorMorph(PIX * pixs, int32 type, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOctreeColorQuant(PIX * pixs, int32 colors, int32 ditherflag);
LEPT_DLL extern PIX * pixOctreeColorQuantGeneral(PIX * pixs,
    int32 colors,
    int32 ditherflag,
    float validthresh,
    float colorthresh);
LEPT_DLL extern l_ok makeRGBToIndexTables(int32 cqlevels, uint32 ** prtab, uint32 ** pgtab, uint32 ** pbtab);
LEPT_DLL extern void getOctcubeIndexFromRGB(int32 rval,
    int32 gval,
    int32 bval,
    uint32 * rtab,
    uint32 * gtab,
    uint32 * btab,
    uint32 * pindex);
LEPT_DLL extern PIX * pixOctreeQuantByPopulation(PIX * pixs, int32 level, int32 ditherflag);
LEPT_DLL extern PIX * pixOctreeQuantNumColors(PIX * pixs, int32 maxcolors, int32 subsample);
LEPT_DLL extern PIX * pixOctcubeQuantMixedWithGray(PIX * pixs, int32 depth, int32 graylevels, int32 delta);
LEPT_DLL extern PIX * pixFixedOctcubeQuant256(PIX * pixs, int32 ditherflag);
LEPT_DLL extern PIX * pixFewColorsOctcubeQuant1(PIX * pixs, int32 level);
LEPT_DLL extern PIX * pixFewColorsOctcubeQuant2(PIX * pixs, int32 level, NUMA * na, int32 ncolors, int32 * pnerrors);
LEPT_DLL extern PIX * pixFewColorsOctcubeQuantMixed(PIX * pixs,
    int32 level,
    int32 darkthresh,
    int32 lightthresh,
    int32 diffthresh,
    float minfract,
    int32 maxspan);
LEPT_DLL extern PIX * pixFixedOctcubeQuantGenRGB(PIX * pixs, int32 level);
LEPT_DLL extern PIX * pixQuantFromCmap(PIX * pixs, PIXCMAP * cmap, int32 mindepth, int32 level, int32 metric);
LEPT_DLL extern PIX * pixOctcubeQuantFromCmap(PIX * pixs, PIXCMAP * cmap, int32 mindepth, int32 level, int32 metric);
LEPT_DLL extern NUMA * pixOctcubeHistogram(PIX * pixs, int32 level, int32 * pncolors);
LEPT_DLL extern int32 * pixcmapToOctcubeLUT(PIXCMAP * cmap, int32 level, int32 metric);
LEPT_DLL extern l_ok pixRemoveUnusedColors(PIX * pixs);
LEPT_DLL extern l_ok pixNumberOccupiedOctcubes(PIX * pix, int32 level, int32 mincount, float minfract, int32 * pncolors);
LEPT_DLL extern PIX * pixMedianCutQuant(PIX * pixs, int32 ditherflag);
LEPT_DLL extern PIX * pixMedianCutQuantGeneral(PIX * pixs,
    int32 ditherflag,
    int32 outdepth,
    int32 maxcolors,
    int32 sigbits,
    int32 maxsub,
    int32 checkbw);
LEPT_DLL extern PIX * pixMedianCutQuantMixed(PIX * pixs,
    int32 ncolor,
    int32 ngray,
    int32 darkthresh,
    int32 lightthresh,
    int32 diffthresh);
LEPT_DLL extern PIX * pixFewColorsMedianCutQuantMixed(PIX * pixs,
    int32 ncolor,
    int32 ngray,
    int32 maxncolors,
    int32 darkthresh,
    int32 lightthresh,
    int32 diffthresh);
LEPT_DLL extern int32 * pixMedianCutHisto(PIX * pixs, int32 sigbits, int32 subsample);
LEPT_DLL extern PIX * pixColorSegment(PIX * pixs,
    int32 maxdist,
    int32 maxcolors,
    int32 selsize,
    int32 finalcolors,
    int32 debugflag);
LEPT_DLL extern PIX * pixColorSegmentCluster(PIX * pixs, int32 maxdist, int32 maxcolors, int32 debugflag);
LEPT_DLL extern l_ok pixAssignToNearestColor(PIX * pixd, PIX * pixs, PIX * pixm, int32 level, int32 * countarray);
LEPT_DLL extern l_ok pixColorSegmentClean(PIX * pixs, int32 selsize, int32 * countarray);
LEPT_DLL extern l_ok pixColorSegmentRemoveColors(PIX * pixd, PIX * pixs, int32 finalcolors);
LEPT_DLL extern PIX * pixConvertRGBToHSV(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixConvertHSVToRGB(PIX * pixd, PIX * pixs);
LEPT_DLL extern l_ok convertRGBToHSV(int32 rval, int32 gval, int32 bval, int32 * phval, int32 * psval, int32 * pvval);
LEPT_DLL extern l_ok convertHSVToRGB(int32 hval, int32 sval, int32 vval, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern l_ok pixcmapConvertRGBToHSV(PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapConvertHSVToRGB(PIXCMAP * cmap);
LEPT_DLL extern PIX * pixConvertRGBToHue(PIX * pixs);
LEPT_DLL extern PIX * pixConvertRGBToSaturation(PIX * pixs);
LEPT_DLL extern PIX * pixConvertRGBToValue(PIX * pixs);
LEPT_DLL extern PIX * pixMakeRangeMaskHS(PIX * pixs, int32 huecenter, int32 huehw, int32 satcenter, int32 sathw,
    int32 regionflag);
LEPT_DLL extern PIX * pixMakeRangeMaskHV(PIX * pixs, int32 huecenter, int32 huehw, int32 valcenter, int32 valhw,
    int32 regionflag);
LEPT_DLL extern PIX * pixMakeRangeMaskSV(PIX * pixs, int32 satcenter, int32 sathw, int32 valcenter, int32 valhw,
    int32 regionflag);
LEPT_DLL extern PIX * pixMakeHistoHS(PIX * pixs, int32 factor, NUMA ** pnahue, NUMA ** pnasat);
LEPT_DLL extern PIX * pixMakeHistoHV(PIX * pixs, int32 factor, NUMA ** pnahue, NUMA ** pnaval);
LEPT_DLL extern PIX * pixMakeHistoSV(PIX * pixs, int32 factor, NUMA ** pnasat, NUMA ** pnaval);
LEPT_DLL extern l_ok pixFindHistoPeaksHSV(PIX * pixs,
    int32 type,
    int32 width,
    int32 height,
    int32 npeaks,
    float erasefactor,
    PTA ** ppta,
    NUMA ** pnatot,
    PIXA ** ppixa);
LEPT_DLL extern PIX * displayHSVColorRange(int32 hval,
    int32 sval,
    int32 vval,
    int32 huehw,
    int32 sathw,
    int32 nsamp,
    int32 factor);
LEPT_DLL extern PIX * pixConvertRGBToYUV(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixConvertYUVToRGB(PIX * pixd, PIX * pixs);
LEPT_DLL extern l_ok convertRGBToYUV(int32 rval, int32 gval, int32 bval, int32 * pyval, int32 * puval, int32 * pvval);
LEPT_DLL extern l_ok convertYUVToRGB(int32 yval, int32 uval, int32 vval, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern l_ok pixcmapConvertRGBToYUV(PIXCMAP * cmap);
LEPT_DLL extern l_ok pixcmapConvertYUVToRGB(PIXCMAP * cmap);
LEPT_DLL extern FPIXA * pixConvertRGBToXYZ(PIX * pixs);
LEPT_DLL extern PIX * fpixaConvertXYZToRGB(FPIXA * fpixa);
LEPT_DLL extern l_ok convertRGBToXYZ(int32 rval, int32 gval, int32 bval, float * pfxval, float * pfyval, float * pfzval);
LEPT_DLL extern l_ok convertXYZToRGB(float fxval,
    float fyval,
    float fzval,
    int32 blackout,
    int32 * prval,
    int32 * pgval,
    int32 * pbval);
LEPT_DLL extern FPIXA * fpixaConvertXYZToLAB(FPIXA * fpixas);
LEPT_DLL extern FPIXA * fpixaConvertLABToXYZ(FPIXA * fpixas);
LEPT_DLL extern l_ok convertXYZToLAB(float xval, float yval, float zval, float * plval, float * paval,
    float * pbval);
LEPT_DLL extern l_ok convertLABToXYZ(float lval, float aval, float bval, float * pxval, float * pyval,
    float * pzval);
LEPT_DLL extern FPIXA * pixConvertRGBToLAB(PIX * pixs);
LEPT_DLL extern PIX * fpixaConvertLABToRGB(FPIXA * fpixa);
LEPT_DLL extern l_ok convertRGBToLAB(int32 rval, int32 gval, int32 bval, float * pflval, float * pfaval, float * pfbval);
LEPT_DLL extern l_ok convertLABToRGB(float flval, float faval, float fbval, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern PIX * pixMakeGamutRGB(int32 scale);
LEPT_DLL extern l_ok pixEqual(PIX * pix1, PIX * pix2, int32 * psame);
LEPT_DLL extern l_ok pixEqualWithAlpha(PIX * pix1, PIX * pix2, int32 use_alpha, int32 * psame);
LEPT_DLL extern l_ok pixEqualWithCmap(PIX * pix1, PIX * pix2, int32 * psame);
LEPT_DLL extern l_ok cmapEqual(PIXCMAP * cmap1, PIXCMAP * cmap2, int32 ncomps, int32 * psame);
LEPT_DLL extern l_ok pixUsesCmapColor(PIX * pixs, int32 * pcolor);
LEPT_DLL extern l_ok pixCorrelationBinary(PIX * pix1, PIX * pix2, float * pval);
LEPT_DLL extern PIX * pixDisplayDiffBinary(PIX * pix1, PIX * pix2);
LEPT_DLL extern l_ok pixCompareBinary(PIX * pix1, PIX * pix2, int32 comptype, float * pfract, PIX ** ppixdiff);
LEPT_DLL extern l_ok pixCompareGrayOrRGB(PIX * pix1,
    PIX * pix2,
    int32 comptype,
    int32 plottype,
    int32 * psame,
    float * pdiff,
    float * prmsdiff,
    PIX ** ppixdiff);
LEPT_DLL extern l_ok pixCompareGray(PIX * pix1,
    PIX * pix2,
    int32 comptype,
    int32 plottype,
    int32 * psame,
    float * pdiff,
    float * prmsdiff,
    PIX ** ppixdiff);
LEPT_DLL extern l_ok pixCompareRGB(PIX * pix1,
    PIX * pix2,
    int32 comptype,
    int32 plottype,
    int32 * psame,
    float * pdiff,
    float * prmsdiff,
    PIX ** ppixdiff);
LEPT_DLL extern l_ok pixCompareTiled(PIX * pix1, PIX * pix2, int32 sx, int32 sy, int32 type, PIX ** ppixdiff);
LEPT_DLL extern NUMA * pixCompareRankDifference(PIX * pix1, PIX * pix2, int32 factor);
LEPT_DLL extern l_ok pixTestForSimilarity(PIX * pix1,
    PIX * pix2,
    int32 factor,
    int32 mindiff,
    float maxfract,
    float maxave,
    int32 * psimilar,
    int32 details);
LEPT_DLL extern l_ok pixGetDifferenceStats(PIX * pix1,
    PIX * pix2,
    int32 factor,
    int32 mindiff,
    float * pfractdiff,
    float * pavediff,
    int32 details);
LEPT_DLL extern NUMA * pixGetDifferenceHistogram(PIX * pix1, PIX * pix2, int32 factor);
LEPT_DLL extern l_ok pixGetPerceptualDiff(PIX * pixs1,
    PIX * pixs2,
    int32 sampling,
    int32 dilation,
    int32 mindiff,
    float * pfract,
    PIX ** ppixdiff1,
    PIX ** ppixdiff2);
LEPT_DLL extern l_ok pixGetPSNR(PIX * pix1, PIX * pix2, int32 factor, float * ppsnr);
LEPT_DLL extern l_ok pixaComparePhotoRegionsByHisto(PIXA * pixa,
    float minratio,
    float textthresh,
    int32 factor,
    int32 n,
    float simthresh,
    NUMA ** pnai,
    float ** pscores,
    PIX ** ppixd,
    int32 debug);
LEPT_DLL extern l_ok pixComparePhotoRegionsByHisto(PIX * pix1,
    PIX * pix2,
    BOX * box1,
    BOX * box2,
    float minratio,
    int32 factor,
    int32 n,
    float * pscore,
    int32 debugflag);
LEPT_DLL extern l_ok pixGenPhotoHistos(PIX * pixs,
    BOX * box,
    int32 factor,
    float thresh,
    int32 n,
    NUMAA ** pnaa,
    int32 * pw,
    int32 * ph,
    int32 debugindex);
LEPT_DLL extern PIX * pixPadToCenterCentroid(PIX * pixs, int32 factor);
LEPT_DLL extern l_ok pixCentroid8(PIX * pixs, int32 factor, float * pcx, float * pcy);
LEPT_DLL extern l_ok pixDecideIfPhotoImage(PIX * pix, int32 factor, float thresh, int32 n, NUMAA ** pnaa, PIXA * pixadebug);
LEPT_DLL extern l_ok compareTilesByHisto(NUMAA * naa1,
    NUMAA * naa2,
    float minratio,
    int32 w1,
    int32 h1,
    int32 w2,
    int32 h2,
    float * pscore,
    PIXA * pixadebug);
LEPT_DLL extern l_ok pixCompareGrayByHisto(PIX * pix1,
    PIX * pix2,
    BOX * box1,
    BOX * box2,
    float minratio,
    int32 maxgray,
    int32 factor,
    int32 n,
    float * pscore,
    int32 debugflag);
LEPT_DLL extern l_ok pixCropAlignedToCentroid(PIX * pix1, PIX * pix2, int32 factor, BOX ** pbox1, BOX ** pbox2);
LEPT_DLL extern uint8 * l_compressGrayHistograms(NUMAA * naa, int32 w, int32 h, size_t * psize);
LEPT_DLL extern NUMAA * l_uncompressGrayHistograms(uint8 * bytea, size_t size, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok pixCompareWithTranslation(PIX * pix1,
    PIX * pix2,
    int32 thresh,
    int32 * pdelx,
    int32 * pdely,
    float * pscore,
    int32 debugflag);
LEPT_DLL extern l_ok pixBestCorrelation(PIX * pix1,
    PIX * pix2,
    int32 area1,
    int32 area2,
    int32 etransx,
    int32 etransy,
    int32 maxshift,
    int32 * tab8,
    int32 * pdelx,
    int32 * pdely,
    float * pscore,
    int32 debugflag);
LEPT_DLL extern BOXA * pixConnComp(PIX * pixs, PIXA ** ppixa, int32 connectivity);
LEPT_DLL extern BOXA * pixConnCompPixa(PIX * pixs, PIXA ** ppixa, int32 connectivity);
LEPT_DLL extern BOXA * pixConnCompBB(PIX * pixs, int32 connectivity);
LEPT_DLL extern l_ok pixCountConnComp(PIX * pixs, int32 connectivity, int32 * pcount);
LEPT_DLL extern int32 nextOnPixelInRaster(PIX * pixs, int32 xstart, int32 ystart, int32 * px, int32 * py);
LEPT_DLL extern BOX * pixSeedfillBB(PIX * pixs, L_STACK * stack, int32 x, int32 y, int32 connectivity);
LEPT_DLL extern BOX * pixSeedfill4BB(PIX * pixs, L_STACK * stack, int32 x, int32 y);
LEPT_DLL extern BOX * pixSeedfill8BB(PIX * pixs, L_STACK * stack, int32 x, int32 y);
LEPT_DLL extern l_ok pixSeedfill(PIX * pixs, L_STACK * stack, int32 x, int32 y, int32 connectivity);
LEPT_DLL extern l_ok pixSeedfill4(PIX * pixs, L_STACK * stack, int32 x, int32 y);
LEPT_DLL extern l_ok pixSeedfill8(PIX * pixs, L_STACK * stack, int32 x, int32 y);
LEPT_DLL extern l_ok convertFilesTo1bpp(const char * dirin,
    const char * substr,
    int32 upscaling,
    int32 thresh,
    int32 firstpage,
    int32 npages,
    const char * dirout,
    int32 outformat);
LEPT_DLL extern PIX * pixBlockconv(PIX * pix, int32 wc, int32 hc);
LEPT_DLL extern PIX * pixBlockconvGray(PIX * pixs, PIX * pixacc, int32 wc, int32 hc);
LEPT_DLL extern PIX * pixBlockconvAccum(PIX * pixs);
LEPT_DLL extern PIX * pixBlockconvGrayUnnormalized(PIX * pixs, int32 wc, int32 hc);
LEPT_DLL extern PIX * pixBlockconvTiled(PIX * pix, int32 wc, int32 hc, int32 nx, int32 ny);
LEPT_DLL extern PIX * pixBlockconvGrayTile(PIX * pixs, PIX * pixacc, int32 wc, int32 hc);
LEPT_DLL extern l_ok pixWindowedStats(PIX * pixs,
    int32 wc,
    int32 hc,
    int32 hasborder,
    PIX ** ppixm,
    PIX ** ppixms,
    FPIX ** pfpixv,
    FPIX ** pfpixrv);
LEPT_DLL extern PIX * pixWindowedMean(PIX * pixs, int32 wc, int32 hc, int32 hasborder, int32 normflag);
LEPT_DLL extern PIX * pixWindowedMeanSquare(PIX * pixs, int32 wc, int32 hc, int32 hasborder);
LEPT_DLL extern l_ok pixWindowedVariance(PIX * pixm, PIX * pixms, FPIX ** pfpixv, FPIX ** pfpixrv);
LEPT_DLL extern DPIX * pixMeanSquareAccum(PIX * pixs);
LEPT_DLL extern PIX * pixBlockrank(PIX * pixs, PIX * pixacc, int32 wc, int32 hc, float rank);
LEPT_DLL extern PIX * pixBlocksum(PIX * pixs, PIX * pixacc, int32 wc, int32 hc);
LEPT_DLL extern PIX * pixCensusTransform(PIX * pixs, int32 halfsize, PIX * pixacc);
LEPT_DLL extern PIX * pixConvolve(PIX * pixs, L_KERNEL * kel, int32 outdepth, int32 normflag);
LEPT_DLL extern PIX * pixConvolveSep(PIX * pixs, L_KERNEL * kelx, L_KERNEL * kely, int32 outdepth, int32 normflag);
LEPT_DLL extern PIX * pixConvolveRGB(PIX * pixs, L_KERNEL * kel);
LEPT_DLL extern PIX * pixConvolveRGBSep(PIX * pixs, L_KERNEL * kelx, L_KERNEL * kely);
LEPT_DLL extern FPIX * fpixConvolve(FPIX * fpixs, L_KERNEL * kel, int32 normflag);
LEPT_DLL extern FPIX * fpixConvolveSep(FPIX * fpixs, L_KERNEL * kelx, L_KERNEL * kely, int32 normflag);
LEPT_DLL extern PIX * pixConvolveWithBias(PIX * pixs, L_KERNEL * kel1, L_KERNEL * kel2, int32 force8, int32 * pbias);
LEPT_DLL extern void l_setConvolveSampling(int32 xfact, int32 yfact);
LEPT_DLL extern PIX * pixAddGaussianNoise(PIX * pixs, float stdev);
LEPT_DLL extern float gaussDistribSampling(void);
LEPT_DLL extern l_ok pixCorrelationScore(PIX * pix1,
    PIX * pix2,
    int32 area1,
    int32 area2,
    float delx,
    float dely,
    int32 maxdiffw,
    int32 maxdiffh,
    int32 * tab,
    float * pscore);
LEPT_DLL extern int32 pixCorrelationScoreThresholded(PIX * pix1,
    PIX * pix2,
    int32 area1,
    int32 area2,
    float delx,
    float dely,
    int32 maxdiffw,
    int32 maxdiffh,
    int32 * tab,
    int32 * downcount,
    float score_threshold);
LEPT_DLL extern l_ok pixCorrelationScoreSimple(PIX * pix1,
    PIX * pix2,
    int32 area1,
    int32 area2,
    float delx,
    float dely,
    int32 maxdiffw,
    int32 maxdiffh,
    int32 * tab,
    float * pscore);
LEPT_DLL extern l_ok pixCorrelationScoreShifted(PIX * pix1,
    PIX * pix2,
    int32 area1,
    int32 area2,
    int32 delx,
    int32 dely,
    int32 * tab,
    float * pscore);
LEPT_DLL extern L_DEWARP * dewarpCreate(PIX * pixs, int32 pageno);
LEPT_DLL extern L_DEWARP * dewarpCreateRef(int32 pageno, int32 refpage);
LEPT_DLL extern void dewarpDestroy(L_DEWARP ** pdew);
LEPT_DLL extern L_DEWARPA * dewarpaCreate(int32 nptrs, int32 sampling, int32 redfactor, int32 minlines, int32 maxdist);
LEPT_DLL extern L_DEWARPA * dewarpaCreateFromPixacomp(PIXAC * pixac, int32 useboth, int32 sampling, int32 minlines, int32 maxdist);
LEPT_DLL extern void dewarpaDestroy(L_DEWARPA ** pdewa);
LEPT_DLL extern l_ok dewarpaDestroyDewarp(L_DEWARPA * dewa, int32 pageno);
LEPT_DLL extern l_ok dewarpaInsertDewarp(L_DEWARPA * dewa, L_DEWARP * dew);
LEPT_DLL extern L_DEWARP * dewarpaGetDewarp(L_DEWARPA * dewa, int32 index);
LEPT_DLL extern l_ok dewarpaSetCurvatures(L_DEWARPA * dewa,
    int32 max_linecurv,
    int32 min_diff_linecurv,
    int32 max_diff_linecurv,
    int32 max_edgecurv,
    int32 max_diff_edgecurv,
    int32 max_edgeslope);
LEPT_DLL extern l_ok dewarpaUseBothArrays(L_DEWARPA * dewa, int32 useboth);
LEPT_DLL extern l_ok dewarpaSetCheckColumns(L_DEWARPA * dewa, int32 check_columns);
LEPT_DLL extern l_ok dewarpaSetMaxDistance(L_DEWARPA * dewa, int32 maxdist);
LEPT_DLL extern L_DEWARP * dewarpRead(const char * filename);
LEPT_DLL extern L_DEWARP * dewarpReadStream(FILE * fp);
LEPT_DLL extern L_DEWARP * dewarpReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok dewarpWrite(const char * filename, L_DEWARP * dew);
LEPT_DLL extern l_ok dewarpWriteStream(FILE * fp, L_DEWARP * dew);
LEPT_DLL extern l_ok dewarpWriteMem(uint8 ** pdata, size_t * psize, L_DEWARP * dew);
LEPT_DLL extern L_DEWARPA * dewarpaRead(const char * filename);
LEPT_DLL extern L_DEWARPA * dewarpaReadStream(FILE * fp);
LEPT_DLL extern L_DEWARPA * dewarpaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok dewarpaWrite(const char * filename, L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaWriteStream(FILE * fp, L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaWriteMem(uint8 ** pdata, size_t * psize, L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpBuildPageModel(L_DEWARP * dew, const char * debugfile);
LEPT_DLL extern l_ok dewarpFindVertDisparity(L_DEWARP * dew, PTAA * ptaa, int32 rotflag);
LEPT_DLL extern l_ok dewarpFindHorizDisparity(L_DEWARP * dew, PTAA * ptaa);
LEPT_DLL extern PTAA * dewarpGetTextlineCenters(PIX * pixs, int32 debugflag);
LEPT_DLL extern PTAA * dewarpRemoveShortLines(PIX * pixs, PTAA * ptaas, float fract, int32 debugflag);
LEPT_DLL extern l_ok dewarpFindHorizSlopeDisparity(L_DEWARP * dew, PIX * pixb, float fractthresh, int32 parity);
LEPT_DLL extern l_ok dewarpBuildLineModel(L_DEWARP * dew, int32 opensize, const char * debugfile);
LEPT_DLL extern l_ok dewarpaModelStatus(L_DEWARPA * dewa, int32 pageno, int32 * pvsuccess, int32 * phsuccess);
LEPT_DLL extern l_ok dewarpaApplyDisparity(L_DEWARPA * dewa, int32 pageno, PIX * pixs, int32 grayin, int32 x, int32 y, PIX ** ppixd, const char * debugfile);
LEPT_DLL extern l_ok dewarpaApplyDisparityBoxa(L_DEWARPA * dewa, int32 pageno, PIX * pixs, BOXA * boxas, int32 mapdir, int32 x, int32 y, BOXA ** pboxad, const char * debugfile);
LEPT_DLL extern l_ok dewarpMinimize(L_DEWARP * dew);
LEPT_DLL extern l_ok dewarpPopulateFullRes(L_DEWARP * dew, PIX * pix, int32 x, int32 y);
LEPT_DLL extern l_ok dewarpSinglePage(PIX * pixs, int32 thresh, int32 adaptive, int32 useboth, int32 check_columns, PIX ** ppixd, L_DEWARPA ** pdewa, int32 debug);
LEPT_DLL extern l_ok dewarpSinglePageInit(PIX * pixs, int32 thresh, int32 adaptive, int32 useboth, int32 check_columns, PIX ** ppixb, L_DEWARPA ** pdewa);
LEPT_DLL extern l_ok dewarpSinglePageRun(PIX * pixs, PIX * pixb, L_DEWARPA * dewa, PIX ** ppixd, int32 debug);
LEPT_DLL extern l_ok dewarpaListPages(L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaSetValidModels(L_DEWARPA * dewa, int32 notests, int32 debug);
LEPT_DLL extern l_ok dewarpaInsertRefModels(L_DEWARPA * dewa, int32 notests, int32 debug);
LEPT_DLL extern l_ok dewarpaStripRefModels(L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaRestoreModels(L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaInfo(FILE * fp, L_DEWARPA * dewa);
LEPT_DLL extern l_ok dewarpaModelStats(L_DEWARPA * dewa, int32 * pnnone, int32 * pnvsuccess, int32 * pnvvalid, int32 * pnhsuccess, int32 * pnhvalid, int32 * pnref);
LEPT_DLL extern l_ok dewarpaShowArrays(L_DEWARPA * dewa, float scalefact, int32 first, int32 last);
LEPT_DLL extern l_ok dewarpDebug(L_DEWARP * dew, const char * subdirs, int32 index);
LEPT_DLL extern l_ok dewarpShowResults(L_DEWARPA * dewa, SARRAY * sa, BOXA * boxa, int32 firstpage, int32 lastpage, const char * pdfout);
LEPT_DLL extern L_DNA * l_dnaCreate(int32 n);
LEPT_DLL extern L_DNA * l_dnaCreateFromIArray(int32 * iarray, int32 size);
LEPT_DLL extern L_DNA * l_dnaCreateFromDArray(double * darray, int32 size, int32 copyflag);
LEPT_DLL extern L_DNA * l_dnaMakeSequence(double startval, double increment, int32 size);
LEPT_DLL extern void l_dnaDestroy(L_DNA ** pda);
LEPT_DLL extern L_DNA * l_dnaCopy(L_DNA * da);
LEPT_DLL extern L_DNA * l_dnaClone(L_DNA * da);
LEPT_DLL extern l_ok l_dnaEmpty(L_DNA * da);
LEPT_DLL extern l_ok l_dnaAddNumber(L_DNA * da, double val);
LEPT_DLL extern l_ok l_dnaInsertNumber(L_DNA * da, int32 index, double val);
LEPT_DLL extern l_ok l_dnaRemoveNumber(L_DNA * da, int32 index);
LEPT_DLL extern l_ok l_dnaReplaceNumber(L_DNA * da, int32 index, double val);
LEPT_DLL extern int32 l_dnaGetCount(L_DNA * da);
LEPT_DLL extern l_ok l_dnaSetCount(L_DNA * da, int32 newcount);
LEPT_DLL extern l_ok l_dnaGetDValue(L_DNA * da, int32 index, double * pval);
LEPT_DLL extern l_ok l_dnaGetIValue(L_DNA * da, int32 index, int32 * pival);
LEPT_DLL extern l_ok l_dnaSetValue(L_DNA * da, int32 index, double val);
LEPT_DLL extern l_ok l_dnaShiftValue(L_DNA * da, int32 index, double diff);
LEPT_DLL extern int32 * l_dnaGetIArray(L_DNA * da);
LEPT_DLL extern double * l_dnaGetDArray(L_DNA * da, int32 copyflag);
LEPT_DLL extern int32 l_dnaGetRefcount(L_DNA * da);
LEPT_DLL extern l_ok l_dnaChangeRefcount(L_DNA * da, int32 delta);
LEPT_DLL extern l_ok l_dnaGetParameters(L_DNA * da, double * pstartx, double * pdelx);
LEPT_DLL extern l_ok l_dnaSetParameters(L_DNA * da, double startx, double delx);
LEPT_DLL extern l_ok l_dnaCopyParameters(L_DNA * dad, L_DNA * das);
LEPT_DLL extern L_DNA * l_dnaRead(const char * filename);
LEPT_DLL extern L_DNA * l_dnaReadStream(FILE * fp);
LEPT_DLL extern L_DNA * l_dnaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok l_dnaWrite(const char * filename, L_DNA * da);
LEPT_DLL extern l_ok l_dnaWriteStream(FILE * fp, L_DNA * da);
LEPT_DLL extern l_ok l_dnaWriteStderr(L_DNA * da);
LEPT_DLL extern l_ok l_dnaWriteMem(uint8 ** pdata, size_t * psize, L_DNA * da);
LEPT_DLL extern L_DNAA * l_dnaaCreate(int32 n);
LEPT_DLL extern L_DNAA * l_dnaaCreateFull(int32 nptr, int32 n);
LEPT_DLL extern l_ok l_dnaaTruncate(L_DNAA * daa);
LEPT_DLL extern void l_dnaaDestroy(L_DNAA ** pdaa);
LEPT_DLL extern l_ok l_dnaaAddDna(L_DNAA * daa, L_DNA * da, int32 copyflag);
LEPT_DLL extern int32 l_dnaaGetCount(L_DNAA * daa);
LEPT_DLL extern int32 l_dnaaGetDnaCount(L_DNAA * daa, int32 index);
LEPT_DLL extern int32 l_dnaaGetNumberCount(L_DNAA * daa);
LEPT_DLL extern L_DNA * l_dnaaGetDna(L_DNAA * daa, int32 index, int32 accessflag);
LEPT_DLL extern l_ok l_dnaaReplaceDna(L_DNAA * daa, int32 index, L_DNA * da);
LEPT_DLL extern l_ok l_dnaaGetValue(L_DNAA * daa, int32 i, int32 j, double * pval);
LEPT_DLL extern l_ok l_dnaaAddNumber(L_DNAA * daa, int32 index, double val);
LEPT_DLL extern L_DNAA * l_dnaaRead(const char * filename);
LEPT_DLL extern L_DNAA * l_dnaaReadStream(FILE * fp);
LEPT_DLL extern L_DNAA * l_dnaaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok l_dnaaWrite(const char * filename, L_DNAA * daa);
LEPT_DLL extern l_ok l_dnaaWriteStream(FILE * fp, L_DNAA * daa);
LEPT_DLL extern l_ok l_dnaaWriteMem(uint8 ** pdata, size_t * psize, L_DNAA * daa);
LEPT_DLL extern l_ok l_dnaJoin(L_DNA * dad, L_DNA * das, int32 istart, int32 iend);
LEPT_DLL extern L_DNA * l_dnaaFlattenToDna(L_DNAA * daa);
LEPT_DLL extern L_DNA * l_dnaSelectRange(L_DNA * das, int32 first, int32 last);
LEPT_DLL extern NUMA * l_dnaConvertToNuma(L_DNA * da);
LEPT_DLL extern L_DNA * numaConvertToDna(NUMA * na);
LEPT_DLL extern L_DNA * pixConvertDataToDna(PIX * pix);
LEPT_DLL extern L_ASET * l_asetCreateFromDna(L_DNA * da);
LEPT_DLL extern l_ok l_dnaRemoveDupsByAset(L_DNA * das, L_DNA ** pdad);
LEPT_DLL extern l_ok l_dnaUnionByAset(L_DNA * da1, L_DNA * da2, L_DNA ** pdad);
LEPT_DLL extern l_ok l_dnaIntersectionByAset(L_DNA * da1, L_DNA * da2, L_DNA ** pdad);
LEPT_DLL extern L_HASHMAP * l_hmapCreateFromDna(L_DNA * da);
LEPT_DLL extern l_ok l_dnaRemoveDupsByHmap(L_DNA * das, L_DNA ** pdad, L_HASHMAP ** phmap);
LEPT_DLL extern l_ok l_dnaUnionByHmap(L_DNA * da1, L_DNA * da2, L_DNA ** pdad);
LEPT_DLL extern l_ok l_dnaIntersectionByHmap(L_DNA * da1, L_DNA * da2, L_DNA ** pdad);
LEPT_DLL extern l_ok l_dnaMakeHistoByHmap(L_DNA * das, L_DNA ** pdav, L_DNA ** pdac);
LEPT_DLL extern L_DNA * l_dnaDiffAdjValues(L_DNA * das);
LEPT_DLL extern L_DNAHASH * l_dnaHashCreate(int32 nbuckets, int32 initsize);
LEPT_DLL extern void l_dnaHashDestroy(L_DNAHASH ** pdahash);
LEPT_DLL extern L_DNA * l_dnaHashGetDna(L_DNAHASH * dahash, uint64 key, int32 copyflag);
LEPT_DLL extern l_ok l_dnaHashAdd(L_DNAHASH * dahash, uint64 key, double value);
LEPT_DLL extern PIX * pixMorphDwa_2(PIX * pixd, PIX * pixs, int32 operation, char * selname);
LEPT_DLL extern PIX * pixFMorphopGen_2(PIX * pixd, PIX * pixs, int32 operation, char * selname);
LEPT_DLL extern int32 fmorphopgen_low_2(uint32 * datad,
    int32 w,
    int32 h,
    int32 wpld,
    uint32 * datas,
    int32 wpls,
    int32 index);
LEPT_DLL extern PIX * pixSobelEdgeFilter(PIX * pixs, int32 orientflag);
LEPT_DLL extern PIX * pixTwoSidedEdgeFilter(PIX * pixs, int32 orientflag);
LEPT_DLL extern l_ok pixMeasureEdgeSmoothness(PIX * pixs,
    int32 side,
    int32 minjump,
    int32 minreversal,
    float * pjpl,
    float * pjspl,
    float * prpl,
    const char * debugfile);
LEPT_DLL extern NUMA * pixGetEdgeProfile(PIX * pixs, int32 side, const char * debugfile);
LEPT_DLL extern l_ok pixGetLastOffPixelInRun(PIX * pixs, int32 x, int32 y, int32 direction, int32 * ploc);
LEPT_DLL extern int32 pixGetLastOnPixelInRun(PIX * pixs, int32 x, int32 y, int32 direction, int32 * ploc);
LEPT_DLL extern char * encodeBase64(const uint8 * inarray, int32 insize, int32 * poutsize);
LEPT_DLL extern uint8 * decodeBase64(const char * inarray, int32 insize, int32 * poutsize);
LEPT_DLL extern char * encodeAscii85(const uint8 * inarray, size_t insize, size_t * poutsize);
LEPT_DLL extern uint8 * decodeAscii85(const char * inarray, size_t insize, size_t * poutsize);
LEPT_DLL extern char * encodeAscii85WithComp(const uint8 * indata, size_t insize, size_t * poutsize);
LEPT_DLL extern uint8 * decodeAscii85WithComp(const char * instr, size_t insize, size_t * poutsize);
LEPT_DLL extern char * reformatPacked64(const char * inarray,
    int32 insize,
    int32 leadspace,
    int32 linechars,
    int32 addquotes,
    int32 * poutsize);
LEPT_DLL extern PIX * pixGammaTRC(PIX * pixd, PIX * pixs, float gamma, int32 minval, int32 maxval);
LEPT_DLL extern PIX * pixGammaTRCMasked(PIX * pixd, PIX * pixs, PIX * pixm, float gamma, int32 minval, int32 maxval);
LEPT_DLL extern PIX * pixGammaTRCWithAlpha(PIX * pixd, PIX * pixs, float gamma, int32 minval, int32 maxval);
LEPT_DLL extern NUMA * numaGammaTRC(float gamma, int32 minval, int32 maxval);
LEPT_DLL extern PIX * pixContrastTRC(PIX * pixd, PIX * pixs, float factor);
LEPT_DLL extern PIX * pixContrastTRCMasked(PIX * pixd, PIX * pixs, PIX * pixm, float factor);
LEPT_DLL extern NUMA * numaContrastTRC(float factor);
LEPT_DLL extern PIX * pixEqualizeTRC(PIX * pixd, PIX * pixs, float fract, int32 factor);
LEPT_DLL extern NUMA * numaEqualizeTRC(PIX * pix, float fract, int32 factor);
LEPT_DLL extern int32 pixTRCMap(PIX * pixs, PIX * pixm, NUMA * na);
LEPT_DLL extern int32 pixTRCMapGeneral(PIX * pixs, PIX * pixm, NUMA * nar, NUMA * nag, NUMA * nab);
LEPT_DLL extern PIX * pixUnsharpMasking(PIX * pixs, int32 halfwidth, float fract);
LEPT_DLL extern PIX * pixUnsharpMaskingGray(PIX * pixs, int32 halfwidth, float fract);
LEPT_DLL extern PIX * pixUnsharpMaskingFast(PIX * pixs, int32 halfwidth, float fract, int32 direction);
LEPT_DLL extern PIX * pixUnsharpMaskingGrayFast(PIX * pixs, int32 halfwidth, float fract, int32 direction);
LEPT_DLL extern PIX * pixUnsharpMaskingGray1D(PIX * pixs, int32 halfwidth, float fract, int32 direction);
LEPT_DLL extern PIX * pixUnsharpMaskingGray2D(PIX * pixs, int32 halfwidth, float fract);
LEPT_DLL extern PIX * pixModifyHue(PIX * pixd, PIX * pixs, float fract);
LEPT_DLL extern PIX * pixModifySaturation(PIX * pixd, PIX * pixs, float fract);
LEPT_DLL extern int32 pixMeasureSaturation(PIX * pixs, int32 factor, float * psat);
LEPT_DLL extern PIX * pixModifyBrightness(PIX * pixd, PIX * pixs, float fract);
LEPT_DLL extern PIX * pixMosaicColorShiftRGB(PIX * pixs, float roff, float goff, float boff, float delta, int32 nincr);
LEPT_DLL extern PIX * pixColorShiftRGB(PIX * pixs, float rfract, float gfract, float bfract);
LEPT_DLL extern PIX * pixDarkenGray(PIX * pixd, PIX * pixs, int32 thresh, int32 satlimit);
LEPT_DLL extern PIX * pixMultConstantColor(PIX * pixs, float rfact, float gfact, float bfact);
LEPT_DLL extern PIX * pixMultMatrixColor(PIX * pixs, L_KERNEL * kel);
LEPT_DLL extern PIX * pixHalfEdgeByBandpass(PIX * pixs, int32 sm1h, int32 sm1v, int32 sm2h, int32 sm2v);
LEPT_DLL extern l_ok fhmtautogen(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern l_ok fhmtautogen1(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern l_ok fhmtautogen2(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern PIX * pixHMTDwa_1(PIX * pixd, PIX * pixs, const char * selname);
LEPT_DLL extern PIX * pixFHMTGen_1(PIX * pixd, PIX * pixs, const char * selname);
LEPT_DLL extern int32 fhmtgen_low_1(uint32 * datad, int32 w, int32 h, int32 wpld, uint32 * datas, int32 wpls, int32 index);
LEPT_DLL extern l_ok pixItalicWords(PIX * pixs, BOXA * boxaw, PIX * pixw, BOXA ** pboxa, int32 debugflag);
LEPT_DLL extern PIX * pixOrientCorrect(PIX * pixs,
    float minupconf,
    float minratio,
    float * pupconf,
    float * pleftconf,
    int32 * protation,
    int32 debug);
LEPT_DLL extern l_ok pixOrientDetect(PIX * pixs, float * pupconf, float * pleftconf, int32 mincount, int32 debug);
LEPT_DLL extern l_ok makeOrientDecision(float upconf,
    float leftconf,
    float minupconf,
    float minratio,
    int32 * porient,
    int32 debug);
LEPT_DLL extern l_ok pixUpDownDetect(PIX * pixs, float * pconf, int32 mincount, int32 npixels, int32 debug);
LEPT_DLL extern l_ok pixMirrorDetect(PIX * pixs, float * pconf, int32 mincount, int32 debug);
LEPT_DLL extern l_ok fmorphautogen(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern l_ok fmorphautogen1(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern int32 fmorphautogen2(SELA * sela, int32 fileindex, const char * filename);
LEPT_DLL extern PIX * pixMorphDwa_1(PIX * pixd, PIX * pixs, int32 operation, char * selname);
LEPT_DLL extern PIX * pixFMorphopGen_1(PIX * pixd, PIX * pixs, int32 operation, char * selname);
LEPT_DLL extern int32 fmorphopgen_low_1(uint32 * datad,
    int32 w,
    int32 h,
    int32 wpld,
    uint32 * datas,
    int32 wpls,
    int32 index);
LEPT_DLL extern FPIX * fpixCreate(int32 width, int32 height);
LEPT_DLL extern FPIX * fpixCreateTemplate(FPIX * fpixs);
LEPT_DLL extern FPIX * fpixClone(FPIX * fpix);
LEPT_DLL extern FPIX * fpixCopy(FPIX * fpixs);
LEPT_DLL extern void fpixDestroy(FPIX ** pfpix);
LEPT_DLL extern l_ok fpixGetDimensions(FPIX * fpix, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok fpixSetDimensions(FPIX * fpix, int32 w, int32 h);
LEPT_DLL extern int32 fpixGetWpl(FPIX * fpix);
LEPT_DLL extern l_ok fpixSetWpl(FPIX * fpix, int32 wpl);
LEPT_DLL extern int32 fpixGetRefcount(FPIX * fpix);
LEPT_DLL extern l_ok fpixChangeRefcount(FPIX * fpix, int32 delta);
LEPT_DLL extern l_ok fpixGetResolution(FPIX * fpix, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok fpixSetResolution(FPIX * fpix, int32 xres, int32 yres);
LEPT_DLL extern l_ok fpixCopyResolution(FPIX * fpixd, FPIX * fpixs);
LEPT_DLL extern float * fpixGetData(FPIX * fpix);
LEPT_DLL extern l_ok fpixSetData(FPIX * fpix, float * data);
LEPT_DLL extern l_ok fpixGetPixel(FPIX * fpix, int32 x, int32 y, float * pval);
LEPT_DLL extern l_ok fpixSetPixel(FPIX * fpix, int32 x, int32 y, float val);
LEPT_DLL extern FPIXA * fpixaCreate(int32 n);
LEPT_DLL extern FPIXA * fpixaCopy(FPIXA * fpixa, int32 copyflag);
LEPT_DLL extern void fpixaDestroy(FPIXA ** pfpixa);
LEPT_DLL extern l_ok fpixaAddFPix(FPIXA * fpixa, FPIX * fpix, int32 copyflag);
LEPT_DLL extern int32 fpixaGetCount(FPIXA * fpixa);
LEPT_DLL extern l_ok fpixaChangeRefcount(FPIXA * fpixa, int32 delta);
LEPT_DLL extern FPIX * fpixaGetFPix(FPIXA * fpixa, int32 index, int32 accesstype);
LEPT_DLL extern l_ok fpixaGetFPixDimensions(FPIXA * fpixa, int32 index, int32 * pw, int32 * ph);
LEPT_DLL extern float * fpixaGetData(FPIXA * fpixa, int32 index);
LEPT_DLL extern l_ok fpixaGetPixel(FPIXA * fpixa, int32 index, int32 x, int32 y, float * pval);
LEPT_DLL extern l_ok fpixaSetPixel(FPIXA * fpixa, int32 index, int32 x, int32 y, float val);
LEPT_DLL extern DPIX * dpixCreate(int32 width, int32 height);
LEPT_DLL extern DPIX * dpixCreateTemplate(DPIX * dpixs);
LEPT_DLL extern DPIX * dpixClone(DPIX * dpix);
LEPT_DLL extern DPIX * dpixCopy(DPIX * dpixs);
LEPT_DLL extern void dpixDestroy(DPIX ** pdpix);
LEPT_DLL extern l_ok dpixGetDimensions(DPIX * dpix, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok dpixSetDimensions(DPIX * dpix, int32 w, int32 h);
LEPT_DLL extern int32 dpixGetWpl(DPIX * dpix);
LEPT_DLL extern l_ok dpixSetWpl(DPIX * dpix, int32 wpl);
LEPT_DLL extern int32 dpixGetRefcount(DPIX * dpix);
LEPT_DLL extern l_ok dpixChangeRefcount(DPIX * dpix, int32 delta);
LEPT_DLL extern l_ok dpixGetResolution(DPIX * dpix, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok dpixSetResolution(DPIX * dpix, int32 xres, int32 yres);
LEPT_DLL extern l_ok dpixCopyResolution(DPIX * dpixd, DPIX * dpixs);
LEPT_DLL extern double * dpixGetData(DPIX * dpix);
LEPT_DLL extern l_ok dpixSetData(DPIX * dpix, double * data);
LEPT_DLL extern l_ok dpixGetPixel(DPIX * dpix, int32 x, int32 y, double * pval);
LEPT_DLL extern l_ok dpixSetPixel(DPIX * dpix, int32 x, int32 y, double val);
LEPT_DLL extern FPIX * fpixRead(const char * filename);
LEPT_DLL extern FPIX * fpixReadStream(FILE * fp);
LEPT_DLL extern FPIX * fpixReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok fpixWrite(const char * filename, FPIX * fpix);
LEPT_DLL extern l_ok fpixWriteStream(FILE * fp, FPIX * fpix);
LEPT_DLL extern l_ok fpixWriteMem(uint8 ** pdata, size_t * psize, FPIX * fpix);
LEPT_DLL extern FPIX * fpixEndianByteSwap(FPIX * fpixd, FPIX * fpixs);
LEPT_DLL extern DPIX * dpixRead(const char * filename);
LEPT_DLL extern DPIX * dpixReadStream(FILE * fp);
LEPT_DLL extern DPIX * dpixReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok dpixWrite(const char * filename, DPIX * dpix);
LEPT_DLL extern l_ok dpixWriteStream(FILE * fp, DPIX * dpix);
LEPT_DLL extern l_ok dpixWriteMem(uint8 ** pdata, size_t * psize, DPIX * dpix);
LEPT_DLL extern DPIX * dpixEndianByteSwap(DPIX * dpixd, DPIX * dpixs);
LEPT_DLL extern l_ok fpixPrintStream(FILE * fp, FPIX * fpix, int32 factor);
LEPT_DLL extern FPIX * pixConvertToFPix(PIX * pixs, int32 ncomps);
LEPT_DLL extern DPIX * pixConvertToDPix(PIX * pixs, int32 ncomps);
LEPT_DLL extern PIX * fpixConvertToPix(FPIX * fpixs, int32 outdepth, int32 negvals, int32 errorflag);
LEPT_DLL extern PIX * fpixDisplayMaxDynamicRange(FPIX * fpixs);
LEPT_DLL extern DPIX * fpixConvertToDPix(FPIX * fpix);
LEPT_DLL extern PIX * dpixConvertToPix(DPIX * dpixs, int32 outdepth, int32 negvals, int32 errorflag);
LEPT_DLL extern FPIX * dpixConvertToFPix(DPIX * dpix);
LEPT_DLL extern l_ok fpixGetMin(FPIX * fpix, float * pminval, int32 * pxminloc, int32 * pyminloc);
LEPT_DLL extern l_ok fpixGetMax(FPIX * fpix, float * pmaxval, int32 * pxmaxloc, int32 * pymaxloc);
LEPT_DLL extern l_ok dpixGetMin(DPIX * dpix, double * pminval, int32 * pxminloc, int32 * pyminloc);
LEPT_DLL extern l_ok dpixGetMax(DPIX * dpix, double * pmaxval, int32 * pxmaxloc, int32 * pymaxloc);
LEPT_DLL extern FPIX * fpixScaleByInteger(FPIX * fpixs, int32 factor);
LEPT_DLL extern DPIX * dpixScaleByInteger(DPIX * dpixs, int32 factor);
LEPT_DLL extern FPIX * fpixLinearCombination(FPIX * fpixd, FPIX * fpixs1, FPIX * fpixs2, float a, float b);
LEPT_DLL extern l_ok fpixAddMultConstant(FPIX * fpix, float addc, float multc);
LEPT_DLL extern DPIX * dpixLinearCombination(DPIX * dpixd, DPIX * dpixs1, DPIX * dpixs2, float a, float b);
LEPT_DLL extern l_ok dpixAddMultConstant(DPIX * dpix, double addc, double multc);
LEPT_DLL extern l_ok fpixSetAllArbitrary(FPIX * fpix, float inval);
LEPT_DLL extern l_ok dpixSetAllArbitrary(DPIX * dpix, double inval);
LEPT_DLL extern FPIX * fpixAddBorder(FPIX * fpixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern FPIX * fpixRemoveBorder(FPIX * fpixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern FPIX * fpixAddMirroredBorder(FPIX * fpixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern FPIX * fpixAddContinuedBorder(FPIX * fpixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern FPIX * fpixAddSlopeBorder(FPIX * fpixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern l_ok fpixRasterop(FPIX * fpixd, int32 dx, int32 dy, int32 dw, int32 dh, FPIX * fpixs, int32 sx, int32 sy);
LEPT_DLL extern FPIX * fpixRotateOrth(FPIX * fpixs, int32 quads);
LEPT_DLL extern FPIX * fpixRotate180(FPIX * fpixd, FPIX * fpixs);
LEPT_DLL extern FPIX * fpixRotate90(FPIX * fpixs, int32 direction);
LEPT_DLL extern FPIX * fpixFlipLR(FPIX * fpixd, FPIX * fpixs);
LEPT_DLL extern FPIX * fpixFlipTB(FPIX * fpixd, FPIX * fpixs);
LEPT_DLL extern FPIX * fpixAffinePta(FPIX * fpixs, PTA * ptad, PTA * ptas, int32 border, float inval);
LEPT_DLL extern FPIX * fpixAffine(FPIX * fpixs, float * vc, float inval);
LEPT_DLL extern FPIX * fpixProjectivePta(FPIX * fpixs, PTA * ptad, PTA * ptas, int32 border, float inval);
LEPT_DLL extern FPIX * fpixProjective(FPIX * fpixs, float * vc, float inval);
LEPT_DLL extern l_ok linearInterpolatePixelFloat(float * datas, int32 w, int32 h, float x, float y, float inval, float * pval);
LEPT_DLL extern PIX * fpixThresholdToPix(FPIX * fpix, float thresh);
LEPT_DLL extern FPIX * pixComponentFunction(PIX * pix, float rnum, float gnum, float bnum, float rdenom, float gdenom, float bdenom);
LEPT_DLL extern PIX * pixReadStreamGif(FILE * fp);
LEPT_DLL extern PIX * pixReadMemGif(const uint8 * cdata, size_t size);
LEPT_DLL extern l_ok pixWriteStreamGif(FILE * fp, PIX * pix);
LEPT_DLL extern l_ok pixWriteMemGif(uint8 ** pdata, size_t * psize, PIX * pix);
LEPT_DLL extern GPLOT * gplotCreate(const char * rootname, int32 outformat, const char * title, const char * xlabel, const char * ylabel);
LEPT_DLL extern void gplotDestroy(GPLOT ** pgplot);
LEPT_DLL extern l_ok gplotAddPlot(GPLOT * gplot, NUMA * nax, NUMA * nay, int32 plotstyle, const char * plotlabel);
LEPT_DLL extern l_ok gplotSetScaling(GPLOT * gplot, int32 scaling);
LEPT_DLL extern PIX * gplotMakeOutputPix(GPLOT * gplot);
LEPT_DLL extern l_ok gplotMakeOutput(GPLOT * gplot);
LEPT_DLL extern l_ok gplotGenCommandFile(GPLOT * gplot);
LEPT_DLL extern l_ok gplotGenDataFiles(GPLOT * gplot);
LEPT_DLL extern l_ok gplotSimple1(NUMA * na, int32 outformat, const char * outroot, const char * title);
LEPT_DLL extern l_ok gplotSimple2(NUMA * na1, NUMA * na2, int32 outformat, const char * outroot, const char * title);
LEPT_DLL extern l_ok gplotSimpleN(NUMAA * naa, int32 outformat, const char * outroot, const char * title);
LEPT_DLL extern PIX * gplotSimplePix1(NUMA * na, const char * title);
LEPT_DLL extern PIX * gplotSimplePix2(NUMA * na1, NUMA * na2, const char * title);
LEPT_DLL extern PIX * gplotSimplePixN(NUMAA * naa, const char * title);
LEPT_DLL extern GPLOT * gplotSimpleXY1(NUMA * nax,
    NUMA * nay,
    int32 plotstyle,
    int32 outformat,
    const char * outroot,
    const char * title);
LEPT_DLL extern GPLOT * gplotSimpleXY2(NUMA * nax, NUMA * nay1, NUMA * nay2, int32 plotstyle, int32 outformat, const char * outroot, const char * title);
LEPT_DLL extern GPLOT * gplotSimpleXYN(NUMA * nax, NUMAA * naay, int32 plotstyle, int32 outformat, const char * outroot, const char * title);
LEPT_DLL extern PIX * gplotGeneralPix1(NUMA * na, int32 plotstyle, const char * rootname, const char * title, const char * xlabel, const char * ylabel);
LEPT_DLL extern PIX * gplotGeneralPix2(NUMA * na1, NUMA * na2, int32 plotstyle, const char * rootname, const char * title, const char * xlabel,
    const char * ylabel);
LEPT_DLL extern PIX * gplotGeneralPixN(NUMA * nax, NUMAA * naay, int32 plotstyle, const char * rootname, const char * title, const char * xlabel, const char * ylabel);
LEPT_DLL extern GPLOT * gplotRead(const char * filename);
LEPT_DLL extern l_ok gplotWrite(const char * filename, GPLOT * gplot);
LEPT_DLL extern PTA * generatePtaLine(int32 x1, int32 y1, int32 x2, int32 y2);
LEPT_DLL extern PTA * generatePtaWideLine(int32 x1, int32 y1, int32 x2, int32 y2, int32 width);
LEPT_DLL extern PTA * generatePtaBox(BOX * box, int32 width);
LEPT_DLL extern PTA * generatePtaBoxa(BOXA * boxa, int32 width, int32 removedups);
LEPT_DLL extern PTA * generatePtaHashBox(BOX * box, int32 spacing, int32 width, int32 orient, int32 outline);
LEPT_DLL extern PTA * generatePtaHashBoxa(BOXA * boxa, int32 spacing, int32 width, int32 orient, int32 outline, int32 removedups);
LEPT_DLL extern PTAA * generatePtaaBoxa(BOXA * boxa);
LEPT_DLL extern PTAA * generatePtaaHashBoxa(BOXA * boxa, int32 spacing, int32 width, int32 orient, int32 outline);
LEPT_DLL extern PTA * generatePtaPolyline(PTA * ptas, int32 width, int32 closeflag, int32 removedups);
LEPT_DLL extern PTA * generatePtaGrid(int32 w, int32 h, int32 nx, int32 ny, int32 width);
LEPT_DLL extern PTA * convertPtaLineTo4cc(PTA * ptas);
LEPT_DLL extern PTA * generatePtaFilledCircle(int32 radius);
LEPT_DLL extern PTA * generatePtaFilledSquare(int32 side);
LEPT_DLL extern PTA * generatePtaLineFromPt(int32 x, int32 y, double length, double radang);
LEPT_DLL extern l_ok locatePtRadially(int32 xr, int32 yr, double dist, double radang, double * px, double * py);
LEPT_DLL extern l_ok pixRenderPlotFromNuma(PIX ** ppix, NUMA * na, int32 plotloc, int32 linewidth, int32 max, uint32 color);
LEPT_DLL extern PTA * makePlotPtaFromNuma(NUMA * na, int32 size, int32 plotloc, int32 linewidth, int32 max);
LEPT_DLL extern l_ok pixRenderPlotFromNumaGen(PIX ** ppix, NUMA * na, int32 orient, int32 linewidth, int32 refpos,
    int32 max, int32 drawref, uint32 color);
LEPT_DLL extern PTA * makePlotPtaFromNumaGen(NUMA * na, int32 orient, int32 linewidth, int32 refpos, int32 max, int32 drawref);
LEPT_DLL extern l_ok pixRenderPta(PIX * pix, PTA * pta, int32 op);
LEPT_DLL extern l_ok pixRenderPtaArb(PIX * pix, PTA * pta, uint8 rval, uint8 gval, uint8 bval);
LEPT_DLL extern l_ok pixRenderPtaBlend(PIX * pix, PTA * pta, uint8 rval, uint8 gval, uint8 bval, float fract);
LEPT_DLL extern l_ok pixRenderLine(PIX * pix, int32 x1, int32 y1, int32 x2, int32 y2, int32 width, int32 op);
LEPT_DLL extern l_ok pixRenderLineArb(PIX * pix, int32 x1, int32 y1, int32 x2, int32 y2,
    int32 width, uint8 rval, uint8 gval, uint8 bval);
LEPT_DLL extern l_ok pixRenderLineBlend(PIX * pix, int32 x1, int32 y1, int32 x2, int32 y2,
    int32 width, uint8 rval, uint8 gval, uint8 bval, float fract);
LEPT_DLL extern l_ok pixRenderBox(PIX * pix, BOX * box, int32 width, int32 op);
LEPT_DLL extern l_ok pixRenderBoxArb(PIX * pix, BOX * box, int32 width, uint8 rval, uint8 gval, uint8 bval);
LEPT_DLL extern l_ok pixRenderBoxBlend(PIX * pix, BOX * box, int32 width, uint8 rval, uint8 gval, uint8 bval, float fract);
LEPT_DLL extern l_ok pixRenderBoxa(PIX * pix, BOXA * boxa, int32 width, int32 op);
LEPT_DLL extern l_ok pixRenderBoxaArb(PIX * pix, BOXA * boxa, int32 width, uint8 rval, uint8 gval, uint8 bval);
LEPT_DLL extern l_ok pixRenderBoxaBlend(PIX * pix, BOXA * boxa, int32 width, uint8 rval, uint8 gval, uint8 bval, float fract, int32 removedups);
LEPT_DLL extern l_ok pixRenderHashBox(PIX * pix, BOX * box, int32 spacing, int32 width, int32 orient, int32 outline, int32 op);
LEPT_DLL extern l_ok pixRenderHashBoxArb(PIX * pix, BOX * box, int32 spacing, int32 width, int32 orient, int32 outline, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixRenderHashBoxBlend(PIX * pix, BOX * box, int32 spacing, int32 width, int32 orient, int32 outline,
    int32 rval, int32 gval, int32 bval, float fract);
LEPT_DLL extern l_ok pixRenderHashMaskArb(PIX * pix, PIX * pixm, int32 x, int32 y, int32 spacing, int32 width, int32 orient,
    int32 outline, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixRenderHashBoxa(PIX * pix, BOXA * boxa, int32 spacing, int32 width, int32 orient, int32 outline, int32 op);
LEPT_DLL extern l_ok pixRenderHashBoxaArb(PIX * pix, BOXA * boxa, int32 spacing, int32 width, int32 orient, int32 outline, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixRenderHashBoxaBlend(PIX * pix, BOXA * boxa, int32 spacing, int32 width, int32 orient, int32 outline, int32 rval, int32 gval, int32 bval, float fract);
LEPT_DLL extern l_ok pixRenderPolyline(PIX * pix, PTA * ptas, int32 width, int32 op, int32 closeflag);
LEPT_DLL extern l_ok pixRenderPolylineArb(PIX * pix, PTA * ptas, int32 width, uint8 rval, uint8 gval, uint8 bval, int32 closeflag);
LEPT_DLL extern l_ok pixRenderPolylineBlend(PIX * pix, PTA * ptas, int32 width, uint8 rval, uint8 gval, uint8 bval, float fract, int32 closeflag, int32 removedups);
LEPT_DLL extern l_ok pixRenderGridArb(PIX * pix, int32 nx, int32 ny, int32 width, uint8 rval, uint8 gval, uint8 bval);
LEPT_DLL extern PIX * pixRenderRandomCmapPtaa(PIX * pix, PTAA * ptaa, int32 polyflag, int32 width, int32 closeflag);
LEPT_DLL extern PIX * pixRenderPolygon(PTA * ptas, int32 width, int32 * pxmin, int32 * pymin);
LEPT_DLL extern PIX * pixFillPolygon(PIX * pixs, PTA * pta, int32 xmin, int32 ymin);
LEPT_DLL extern PIX * pixRenderContours(PIX * pixs, int32 startval, int32 incr, int32 outdepth);
LEPT_DLL extern PIX * fpixAutoRenderContours(FPIX * fpix, int32 ncontours);
LEPT_DLL extern PIX * fpixRenderContours(FPIX * fpixs, float incr, float proxim);
LEPT_DLL extern PTA * pixGeneratePtaBoundary(PIX * pixs, int32 width);
LEPT_DLL extern PIX * pixErodeGray(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixDilateGray(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenGray(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseGray(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeGray3(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixDilateGray3(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenGray3(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseGray3(PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixDitherToBinary(PIX * pixs);
LEPT_DLL extern PIX * pixDitherToBinarySpec(PIX * pixs, int32 lowerclip, int32 upperclip);
LEPT_DLL extern void ditherToBinaryLineLow(uint32 * lined, int32 w, uint32 * bufs1, uint32 * bufs2, int32 lowerclip, int32 upperclip, int32 lastlineflag);
LEPT_DLL extern PIX * pixThresholdToBinary(PIX * pixs, int32 thresh);
LEPT_DLL extern void thresholdToBinaryLineLow(uint32 * lined, int32 w, uint32 * lines, int32 d, int32 thresh);
LEPT_DLL extern PIX * pixVarThresholdToBinary(PIX * pixs, PIX * pixg);
LEPT_DLL extern PIX * pixAdaptThresholdToBinary(PIX * pixs, PIX * pixm, float gamma);
LEPT_DLL extern PIX * pixAdaptThresholdToBinaryGen(PIX * pixs, PIX * pixm, float gamma, int32 blackval, int32 whiteval, int32 thresh);
LEPT_DLL extern PIX * pixGenerateMaskByValue(PIX * pixs, int32 val, int32 usecmap);
LEPT_DLL extern PIX * pixGenerateMaskByBand(PIX * pixs, int32 lower, int32 upper, int32 inband, int32 usecmap);
LEPT_DLL extern PIX * pixDitherTo2bpp(PIX * pixs, int32 cmapflag);
LEPT_DLL extern PIX * pixDitherTo2bppSpec(PIX * pixs, int32 lowerclip, int32 upperclip, int32 cmapflag);
LEPT_DLL extern PIX * pixThresholdTo2bpp(PIX * pixs, int32 nlevels, int32 cmapflag);
LEPT_DLL extern PIX * pixThresholdTo4bpp(PIX * pixs, int32 nlevels, int32 cmapflag);
LEPT_DLL extern PIX * pixThresholdOn8bpp(PIX * pixs, int32 nlevels, int32 cmapflag);
LEPT_DLL extern PIX * pixThresholdGrayArb(PIX * pixs, const char * edgevals, int32 outdepth, int32 use_average, int32 setblack, int32 setwhite);
LEPT_DLL extern int32 * makeGrayQuantIndexTable(int32 nlevels);
LEPT_DLL extern l_ok makeGrayQuantTableArb(NUMA * na, int32 outdepth, int32 ** ptab, PIXCMAP ** pcmap);
LEPT_DLL extern PIX * pixGenerateMaskByBand32(PIX * pixs, uint32 refval, int32 delm, int32 delp, float fractm, float fractp);
LEPT_DLL extern PIX * pixGenerateMaskByDiscr32(PIX * pixs, uint32 refval1, uint32 refval2, int32 distflag);
LEPT_DLL extern PIX * pixGrayQuantFromHisto(PIX * pixd, PIX * pixs, PIX * pixm, float minfract, int32 maxsize);
LEPT_DLL extern PIX * pixGrayQuantFromCmap(PIX * pixs, PIXCMAP * cmap, int32 mindepth);
LEPT_DLL extern L_HASHMAP * l_hmapCreate(int32 ninit, int32 maxocc);
LEPT_DLL extern void l_hmapDestroy(L_HASHMAP ** phmap);
LEPT_DLL extern L_HASHITEM * l_hmapLookup(L_HASHMAP * hmap, uint64 key, uint64 val, int32 op);
LEPT_DLL extern l_ok l_hmapRehash(L_HASHMAP * hmap);
LEPT_DLL extern L_HEAP * lheapCreate(int32 n, int32 direction);
LEPT_DLL extern void lheapDestroy(L_HEAP ** plh, int32 freeflag);
LEPT_DLL extern l_ok lheapAdd(L_HEAP * lh, void * item);
LEPT_DLL extern void * lheapRemove(L_HEAP * lh);
LEPT_DLL extern int32 lheapGetCount(L_HEAP * lh);
LEPT_DLL extern void * lheapGetElement(L_HEAP * lh, int32 index);
LEPT_DLL extern l_ok lheapSort(L_HEAP * lh);
LEPT_DLL extern l_ok lheapSortStrictOrder(L_HEAP * lh);
LEPT_DLL extern l_ok lheapPrint(FILE * fp, L_HEAP * lh);
LEPT_DLL extern JBCLASSER * jbRankHausInit(int32 components, int32 maxwidth, int32 maxheight, int32 size, float rank);
LEPT_DLL extern JBCLASSER * jbCorrelationInit(int32 components, int32 maxwidth, int32 maxheight, float thresh, float weightfactor);
LEPT_DLL extern JBCLASSER * jbCorrelationInitWithoutComponents(int32 components, int32 maxwidth, int32 maxheight, float thresh, float weightfactor);
LEPT_DLL extern l_ok jbAddPages(JBCLASSER * classer, SARRAY * safiles);
LEPT_DLL extern l_ok jbAddPage(JBCLASSER * classer, PIX * pixs);
LEPT_DLL extern l_ok jbAddPageComponents(JBCLASSER * classer, PIX * pixs, BOXA * boxas, PIXA * pixas);
LEPT_DLL extern l_ok jbClassifyRankHaus(JBCLASSER * classer, BOXA * boxa, PIXA * pixas);
LEPT_DLL extern int32 pixHaustest(PIX * pix1,
    PIX * pix2,
    PIX * pix3,
    PIX * pix4,
    float delx,
    float dely,
    int32 maxdiffw,
    int32 maxdiffh);
LEPT_DLL extern int32 pixRankHaustest(PIX * pix1, PIX * pix2, PIX * pix3, PIX * pix4, float delx, float dely, int32 maxdiffw, int32 maxdiffh, int32 area1, int32 area3, float rank, int32 * tab8);
LEPT_DLL extern l_ok jbClassifyCorrelation(JBCLASSER * classer, BOXA * boxa, PIXA * pixas);
LEPT_DLL extern l_ok jbGetComponents(PIX * pixs, int32 components, int32 maxwidth, int32 maxheight, BOXA ** pboxad, PIXA ** ppixad);
LEPT_DLL extern l_ok pixWordMaskByDilation(PIX * pixs, PIX ** ppixm, int32 * psize, PIXA * pixadb);
LEPT_DLL extern l_ok pixWordBoxesByDilation(PIX * pixs, int32 minwidth, int32 minheight, int32 maxwidth, int32 maxheight, BOXA ** pboxa, int32 * psize, PIXA * pixadb);
LEPT_DLL extern PIXA * jbAccumulateComposites(PIXAA * pixaa, NUMA ** pna, PTA ** pptat);
LEPT_DLL extern PIXA * jbTemplatesFromComposites(PIXA * pixac, NUMA * na);
LEPT_DLL extern JBCLASSER * jbClasserCreate(int32 method, int32 components);
LEPT_DLL extern void jbClasserDestroy(JBCLASSER ** pclasser);
LEPT_DLL extern JBDATA * jbDataSave(JBCLASSER * classer);
LEPT_DLL extern void jbDataDestroy(JBDATA ** pdata);
LEPT_DLL extern l_ok jbDataWrite(const char * rootout, JBDATA * jbdata);
LEPT_DLL extern JBDATA * jbDataRead(const char * rootname);
LEPT_DLL extern PIXA * jbDataRender(JBDATA * data, int32 debugflag);
LEPT_DLL extern l_ok jbGetULCorners(JBCLASSER * classer, PIX * pixs, BOXA * boxa);
LEPT_DLL extern l_ok jbGetLLCorners(JBCLASSER * classer);
LEPT_DLL extern l_ok readHeaderJp2k(const char * filename, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pcodec);
LEPT_DLL extern l_ok freadHeaderJp2k(FILE * fp, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pcodec);
LEPT_DLL extern l_ok readHeaderMemJp2k(const uint8 * data, size_t size, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pcodec);
LEPT_DLL extern int32 fgetJp2kResolution(FILE * fp, int32 * pxres, int32 * pyres);
LEPT_DLL extern PIX * pixReadJp2k(const char * filename, uint32 reduction, BOX * box, int32 hint, int32 debug);
LEPT_DLL extern PIX * pixReadStreamJp2k(FILE * fp, uint32 reduction, BOX * box, int32 hint, int32 debug);
LEPT_DLL extern l_ok pixWriteJp2k(const char * filename, PIX * pix, int32 quality, int32 nlevels, int32 hint, int32 debug);
LEPT_DLL extern l_ok pixWriteStreamJp2k(FILE * fp, PIX * pix, int32 quality, int32 nlevels, int32 codec, int32 hint, int32 debug);
LEPT_DLL extern PIX * pixReadMemJp2k(const uint8 * data, size_t size, uint32 reduction, BOX * box, int32 hint, int32 debug);
LEPT_DLL extern l_ok pixWriteMemJp2k(uint8 ** pdata, size_t * psize, PIX * pix, int32 quality, int32 nlevels, int32 hint, int32 debug);
LEPT_DLL extern PIX * pixReadJpeg(const char * filename, int32 cmapflag, int32 reduction, int32 * pnwarn, int32 hint);
LEPT_DLL extern PIX * pixReadStreamJpeg(FILE * fp, int32 cmapflag, int32 reduction, int32 * pnwarn, int32 hint);
LEPT_DLL extern l_ok readHeaderJpeg(const char * filename, int32 * pw, int32 * ph, int32 * pspp, int32 * pycck, int32 * pcmyk);
LEPT_DLL extern l_ok freadHeaderJpeg(FILE * fp, int32 * pw, int32 * ph, int32 * pspp, int32 * pycck, int32 * pcmyk);
LEPT_DLL extern int32 fgetJpegResolution(FILE * fp, int32 * pxres, int32 * pyres);
LEPT_DLL extern int32 fgetJpegComment(FILE * fp, uint8 ** pcomment);
LEPT_DLL extern l_ok pixWriteJpeg(const char * filename, PIX * pix, int32 quality, int32 progressive);
LEPT_DLL extern l_ok pixWriteStreamJpeg(FILE * fp, PIX * pixs, int32 quality, int32 progressive);
LEPT_DLL extern PIX * pixReadMemJpeg(const uint8 * data, size_t size, int32 cmflag, int32 reduction, int32 * pnwarn, int32 hint);
LEPT_DLL extern l_ok readHeaderMemJpeg(const uint8 * data, size_t size, int32 * pw, int32 * ph, int32 * pspp, int32 * pycck, int32 * pcmyk);
LEPT_DLL extern l_ok readResolutionMemJpeg(const uint8 * data, size_t size, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok pixWriteMemJpeg(uint8 ** pdata, size_t * psize, PIX * pix, int32 quality, int32 progressive);
LEPT_DLL extern l_ok pixSetChromaSampling(PIX * pix, int32 sampling);
LEPT_DLL extern L_KERNEL * kernelCreate(int32 height, int32 width);
LEPT_DLL extern void kernelDestroy(L_KERNEL ** pkel);
LEPT_DLL extern L_KERNEL * kernelCopy(L_KERNEL * kels);
LEPT_DLL extern l_ok kernelGetElement(L_KERNEL * kel, int32 row, int32 col, float * pval);
LEPT_DLL extern l_ok kernelSetElement(L_KERNEL * kel, int32 row, int32 col, float val);
LEPT_DLL extern l_ok kernelGetParameters(L_KERNEL * kel, int32 * psy, int32 * psx, int32 * pcy, int32 * pcx);
LEPT_DLL extern l_ok kernelSetOrigin(L_KERNEL * kel, int32 cy, int32 cx);
LEPT_DLL extern l_ok kernelGetSum(L_KERNEL * kel, float * psum);
LEPT_DLL extern l_ok kernelGetMinMax(L_KERNEL * kel, float * pmin, float * pmax);
LEPT_DLL extern L_KERNEL * kernelNormalize(L_KERNEL * kels, float normsum);
LEPT_DLL extern L_KERNEL * kernelInvert(L_KERNEL * kels);
LEPT_DLL extern float ** create2dFloatArray(int32 sy, int32 sx);
LEPT_DLL extern L_KERNEL * kernelRead(const char * fname);
LEPT_DLL extern L_KERNEL * kernelReadStream(FILE * fp);
LEPT_DLL extern l_ok kernelWrite(const char * fname, L_KERNEL * kel);
LEPT_DLL extern l_ok kernelWriteStream(FILE * fp, L_KERNEL * kel);
LEPT_DLL extern L_KERNEL * kernelCreateFromString(int32 h, int32 w, int32 cy, int32 cx, const char * kdata);
LEPT_DLL extern L_KERNEL * kernelCreateFromFile(const char * filename);
LEPT_DLL extern L_KERNEL * kernelCreateFromPix(PIX * pix, int32 cy, int32 cx);
LEPT_DLL extern PIX * kernelDisplayInPix(L_KERNEL * kel, int32 size, int32 gthick);
LEPT_DLL extern NUMA * parseStringForNumbers(const char * str, const char * seps);
LEPT_DLL extern L_KERNEL * makeFlatKernel(int32 height, int32 width, int32 cy, int32 cx);
LEPT_DLL extern L_KERNEL * makeGaussianKernel(int32 halfh, int32 halfw, float stdev, float max);
LEPT_DLL extern l_ok makeGaussianKernelSep(int32 halfh, int32 halfw, float stdev, float max, L_KERNEL ** pkelx, L_KERNEL ** pkely);
LEPT_DLL extern L_KERNEL * makeDoGKernel(int32 halfh, int32 halfw, float stdev, float ratio);
LEPT_DLL extern char * getImagelibVersions(void);
LEPT_DLL extern void listDestroy(DLLIST ** phead);
LEPT_DLL extern l_ok listAddToHead(DLLIST ** phead, void * data);
LEPT_DLL extern l_ok listAddToTail(DLLIST ** phead, DLLIST ** ptail, void * data);
LEPT_DLL extern l_ok listInsertBefore(DLLIST ** phead, DLLIST * elem, void * data);
LEPT_DLL extern l_ok listInsertAfter(DLLIST ** phead, DLLIST * elem, void * data);
LEPT_DLL extern void * listRemoveElement(DLLIST ** phead, DLLIST * elem);
LEPT_DLL extern void * listRemoveFromHead(DLLIST ** phead);
LEPT_DLL extern void * listRemoveFromTail(DLLIST ** phead, DLLIST ** ptail);
LEPT_DLL extern DLLIST * listFindElement(DLLIST * head, void * data);
LEPT_DLL extern DLLIST * listFindTail(DLLIST * head);
LEPT_DLL extern int32 listGetCount(DLLIST * head);
LEPT_DLL extern l_ok listReverse(DLLIST ** phead);
LEPT_DLL extern l_ok listJoin(DLLIST ** phead1, DLLIST ** phead2);
LEPT_DLL extern L_AMAP * l_amapCreate(int32 keytype);
LEPT_DLL extern RB_TYPE * l_amapFind(L_AMAP * m, RB_TYPE key);
LEPT_DLL extern void l_amapInsert(L_AMAP * m, RB_TYPE key, RB_TYPE value);
LEPT_DLL extern void l_amapDelete(L_AMAP * m, RB_TYPE key);
LEPT_DLL extern void l_amapDestroy(L_AMAP ** pm);
LEPT_DLL extern L_AMAP_NODE * l_amapGetFirst(L_AMAP * m);
LEPT_DLL extern L_AMAP_NODE * l_amapGetNext(L_AMAP_NODE * n);
LEPT_DLL extern L_AMAP_NODE * l_amapGetLast(L_AMAP * m);
LEPT_DLL extern L_AMAP_NODE * l_amapGetPrev(L_AMAP_NODE * n);
LEPT_DLL extern int32 l_amapSize(L_AMAP * m);
LEPT_DLL extern L_ASET * l_asetCreate(int32 keytype);
LEPT_DLL extern RB_TYPE * l_asetFind(L_ASET * s, RB_TYPE key);
LEPT_DLL extern void l_asetInsert(L_ASET * s, RB_TYPE key);
LEPT_DLL extern void l_asetDelete(L_ASET * s, RB_TYPE key);
LEPT_DLL extern void l_asetDestroy(L_ASET ** ps);
LEPT_DLL extern L_ASET_NODE * l_asetGetFirst(L_ASET * s);
LEPT_DLL extern L_ASET_NODE * l_asetGetNext(L_ASET_NODE * n);
LEPT_DLL extern L_ASET_NODE * l_asetGetLast(L_ASET * s);
LEPT_DLL extern L_ASET_NODE * l_asetGetPrev(L_ASET_NODE * n);
LEPT_DLL extern int32 l_asetSize(L_ASET * s);
LEPT_DLL extern PIX * generateBinaryMaze(int32 w, int32 h, int32 xi, int32 yi, float wallps, float ranis);
LEPT_DLL extern PTA * pixSearchBinaryMaze(PIX * pixs, int32 xi, int32 yi, int32 xf, int32 yf, PIX ** ppixd);
LEPT_DLL extern PTA * pixSearchGrayMaze(PIX * pixs, int32 xi, int32 yi, int32 xf, int32 yf, PIX ** ppixd);
LEPT_DLL extern PIX * pixDilate(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixErode(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixHMT(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixOpen(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixClose(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixCloseSafe(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixOpenGeneralized(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixCloseGeneralized(PIX * pixd, PIX * pixs, SEL * sel);
LEPT_DLL extern PIX * pixDilateBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseSafeBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern int32 selectComposableSels(int32 size, int32 direction, SEL ** psel1, SEL ** psel2);
LEPT_DLL extern l_ok selectComposableSizes(int32 size, int32 * pfactor1, int32 * pfactor2);
LEPT_DLL extern PIX * pixDilateCompBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeCompBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenCompBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseCompBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseSafeCompBrick(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern void resetMorphBoundaryCondition(int32 bc);
LEPT_DLL extern uint32 getMorphBorderPixelColor(int32 type, int32 depth);
LEPT_DLL extern PIX * pixExtractBoundary(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixMorphSequenceMasked(PIX * pixs, PIX * pixm, const char * sequence, int32 dispsep);
LEPT_DLL extern PIX * pixMorphSequenceByComponent(PIX * pixs, const char * sequence, int32 connectivity, int32 minw, int32 minh, BOXA ** pboxa);
LEPT_DLL extern PIXA * pixaMorphSequenceByComponent(PIXA * pixas, const char * sequence, int32 minw, int32 minh);
LEPT_DLL extern PIX * pixMorphSequenceByRegion(PIX * pixs, PIX * pixm, const char * sequence, int32 connectivity, int32 minw, int32 minh, BOXA ** pboxa);
LEPT_DLL extern PIXA * pixaMorphSequenceByRegion(PIX * pixs, PIXA * pixam, const char * sequence, int32 minw, int32 minh);
LEPT_DLL extern PIX * pixUnionOfMorphOps(PIX * pixs, SELA * sela, int32 type);
LEPT_DLL extern PIX * pixIntersectionOfMorphOps(PIX * pixs, SELA * sela, int32 type);
LEPT_DLL extern PIX * pixSelectiveConnCompFill(PIX * pixs, int32 connectivity, int32 minw, int32 minh);
LEPT_DLL extern l_ok pixRemoveMatchedPattern(PIX * pixs, PIX * pixp, PIX * pixe, int32 x0, int32 y0, int32 dsize);
LEPT_DLL extern PIX * pixDisplayMatchedPattern(PIX * pixs, PIX * pixp, PIX * pixe, int32 x0, int32 y0, uint32 color, float scale, int32 nlevels);
LEPT_DLL extern PIXA * pixaExtendByMorph(PIXA * pixas, int32 type, int32 niters, SEL * sel, int32 include);
LEPT_DLL extern PIXA * pixaExtendByScaling(PIXA * pixas, NUMA * nasc, int32 type, int32 include);
LEPT_DLL extern PIX * pixSeedfillMorph(PIX * pixs, PIX * pixm, int32 maxiters, int32 connectivity);
LEPT_DLL extern NUMA * pixRunHistogramMorph(PIX * pixs, int32 runtype, int32 direction, int32 maxsize);
LEPT_DLL extern PIX * pixTophat(PIX * pixs, int32 hsize, int32 vsize, int32 type);
LEPT_DLL extern PIX * pixHDome(PIX * pixs, int32 height, int32 connectivity);
LEPT_DLL extern PIX * pixFastTophat(PIX * pixs, int32 xsize, int32 ysize, int32 type);
LEPT_DLL extern PIX * pixMorphGradient(PIX * pixs, int32 hsize, int32 vsize, int32 smoothing);
LEPT_DLL extern PTA * pixaCentroids(PIXA * pixa);
LEPT_DLL extern l_ok pixCentroid(PIX * pix, int32 * centtab, int32 * sumtab, float * pxave, float * pyave);
LEPT_DLL extern PIX * pixDilateBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixDilateCompBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeCompBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenCompBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseCompBrickDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixDilateCompBrickExtendDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixErodeCompBrickExtendDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixOpenCompBrickExtendDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern PIX * pixCloseCompBrickExtendDwa(PIX * pixd, PIX * pixs, int32 hsize, int32 vsize);
LEPT_DLL extern l_ok getExtendedCompositeParameters(int32 size, int32 * pn, int32 * pextra, int32 * pactualsize);
LEPT_DLL extern PIX * pixMorphSequence(PIX * pixs, const char * sequence, int32 dispsep);
LEPT_DLL extern PIX * pixMorphCompSequence(PIX * pixs, const char * sequence, int32 dispsep);
LEPT_DLL extern PIX * pixMorphSequenceDwa(PIX * pixs, const char * sequence, int32 dispsep);
LEPT_DLL extern PIX * pixMorphCompSequenceDwa(PIX * pixs, const char * sequence, int32 dispsep);
LEPT_DLL extern int32 morphSequenceVerify(SARRAY * sa);
LEPT_DLL extern PIX * pixGrayMorphSequence(PIX * pixs, const char * sequence, int32 dispsep, int32 dispy);
LEPT_DLL extern PIX * pixColorMorphSequence(PIX * pixs, const char * sequence, int32 dispsep, int32 dispy);
LEPT_DLL extern NUMA * numaCreate(int32 n);
LEPT_DLL extern NUMA * numaCreateFromIArray(int32 * iarray, int32 size);
LEPT_DLL extern NUMA * numaCreateFromFArray(float * farray, int32 size, int32 copyflag);
LEPT_DLL extern NUMA * numaCreateFromString(const char * str);
LEPT_DLL extern void numaDestroy(NUMA ** pna);
LEPT_DLL extern NUMA * numaCopy(NUMA * na);
LEPT_DLL extern NUMA * numaClone(NUMA * na);
LEPT_DLL extern l_ok numaEmpty(NUMA * na);
LEPT_DLL extern l_ok numaAddNumber(NUMA * na, float val);
LEPT_DLL extern l_ok numaInsertNumber(NUMA * na, int32 index, float val);
LEPT_DLL extern l_ok numaRemoveNumber(NUMA * na, int32 index);
LEPT_DLL extern l_ok numaReplaceNumber(NUMA * na, int32 index, float val);
LEPT_DLL extern int32 numaGetCount(NUMA * na);
LEPT_DLL extern l_ok numaSetCount(NUMA * na, int32 newcount);
LEPT_DLL extern l_ok numaGetFValue(NUMA * na, int32 index, float * pval);
LEPT_DLL extern l_ok numaGetIValue(NUMA * na, int32 index, int32 * pival);
LEPT_DLL extern l_ok numaSetValue(NUMA * na, int32 index, float val);
LEPT_DLL extern l_ok numaShiftValue(NUMA * na, int32 index, float diff);
LEPT_DLL extern int32 * numaGetIArray(NUMA * na);
LEPT_DLL extern float * numaGetFArray(NUMA * na, int32 copyflag);
LEPT_DLL extern int32 numaGetRefcount(NUMA * na);
LEPT_DLL extern l_ok numaChangeRefcount(NUMA * na, int32 delta);
LEPT_DLL extern l_ok numaGetParameters(NUMA * na, float * pstartx, float * pdelx);
LEPT_DLL extern l_ok numaSetParameters(NUMA * na, float startx, float delx);
LEPT_DLL extern l_ok numaCopyParameters(NUMA * nad, NUMA * nas);
LEPT_DLL extern SARRAY * numaConvertToSarray(NUMA * na, int32 size1, int32 size2, int32 addzeros, int32 type);
LEPT_DLL extern NUMA * numaRead(const char * filename);
LEPT_DLL extern NUMA * numaReadStream(FILE * fp);
LEPT_DLL extern NUMA * numaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok numaWriteDebug(const char * filename, NUMA * na);
LEPT_DLL extern l_ok numaWrite(const char * filename, NUMA * na);
LEPT_DLL extern l_ok numaWriteStream(FILE * fp, NUMA * na);
LEPT_DLL extern l_ok numaWriteStderr(NUMA * na);
LEPT_DLL extern l_ok numaWriteMem(uint8 ** pdata, size_t * psize, NUMA * na);
LEPT_DLL extern NUMAA * numaaCreate(int32 n);
LEPT_DLL extern NUMAA * numaaCreateFull(int32 nptr, int32 n);
LEPT_DLL extern l_ok numaaTruncate(NUMAA * naa);
LEPT_DLL extern void numaaDestroy(NUMAA ** pnaa);
LEPT_DLL extern l_ok numaaAddNuma(NUMAA * naa, NUMA * na, int32 copyflag);
LEPT_DLL extern int32 numaaGetCount(NUMAA * naa);
LEPT_DLL extern int32 numaaGetNumaCount(NUMAA * naa, int32 index);
LEPT_DLL extern int32 numaaGetNumberCount(NUMAA * naa);
LEPT_DLL extern NUMA ** numaaGetPtrArray(NUMAA * naa);
LEPT_DLL extern NUMA * numaaGetNuma(NUMAA * naa, int32 index, int32 accessflag);
LEPT_DLL extern l_ok numaaReplaceNuma(NUMAA * naa, int32 index, NUMA * na);
LEPT_DLL extern l_ok numaaGetValue(NUMAA * naa, int32 i, int32 j, float * pfval, int32 * pival);
LEPT_DLL extern l_ok numaaAddNumber(NUMAA * naa, int32 index, float val);
LEPT_DLL extern NUMAA * numaaRead(const char * filename);
LEPT_DLL extern NUMAA * numaaReadStream(FILE * fp);
LEPT_DLL extern NUMAA * numaaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok numaaWrite(const char * filename, NUMAA * naa);
LEPT_DLL extern l_ok numaaWriteStream(FILE * fp, NUMAA * naa);
LEPT_DLL extern l_ok numaaWriteMem(uint8 ** pdata, size_t * psize, NUMAA * naa);
LEPT_DLL extern NUMA * numaArithOp(NUMA * nad, NUMA * na1, NUMA * na2, int32 op);
LEPT_DLL extern NUMA * numaLogicalOp(NUMA * nad, NUMA * na1, NUMA * na2, int32 op);
LEPT_DLL extern NUMA * numaInvert(NUMA * nad, NUMA * nas);
LEPT_DLL extern int32 numaSimilar(NUMA * na1, NUMA * na2, float maxdiff, int32 * psimilar);
LEPT_DLL extern l_ok numaAddToNumber(NUMA * na, int32 index, float val);
LEPT_DLL extern l_ok numaGetMin(NUMA * na, float * pminval, int32 * piminloc);
LEPT_DLL extern l_ok numaGetMax(NUMA * na, float * pmaxval, int32 * pimaxloc);
LEPT_DLL extern l_ok numaGetSum(NUMA * na, float * psum);
LEPT_DLL extern NUMA * numaGetPartialSums(NUMA * na);
LEPT_DLL extern l_ok numaGetSumOnInterval(NUMA * na, int32 first, int32 last, float * psum);
LEPT_DLL extern l_ok numaHasOnlyIntegers(NUMA * na, int32 * pallints);
LEPT_DLL extern l_ok numaGetMean(NUMA * na, float * pave);
LEPT_DLL extern l_ok numaGetMeanAbsval(NUMA * na, float * paveabs);
LEPT_DLL extern NUMA * numaSubsample(NUMA * nas, int32 subfactor);
LEPT_DLL extern NUMA * numaMakeDelta(NUMA * nas);
LEPT_DLL extern NUMA * numaMakeSequence(float startval, float increment, int32 size);
LEPT_DLL extern NUMA * numaMakeConstant(float val, int32 size);
LEPT_DLL extern NUMA * numaMakeAbsval(NUMA * nad, NUMA * nas);
LEPT_DLL extern NUMA * numaAddBorder(NUMA * nas, int32 left, int32 right, float val);
LEPT_DLL extern NUMA * numaAddSpecifiedBorder(NUMA * nas, int32 left, int32 right, int32 type);
LEPT_DLL extern NUMA * numaRemoveBorder(NUMA * nas, int32 left, int32 right);
LEPT_DLL extern l_ok numaCountNonzeroRuns(NUMA * na, int32 * pcount);
LEPT_DLL extern l_ok numaGetNonzeroRange(NUMA * na, float eps, int32 * pfirst, int32 * plast);
LEPT_DLL extern l_ok numaGetCountRelativeToZero(NUMA * na, int32 type, int32 * pcount);
LEPT_DLL extern NUMA * numaClipToInterval(NUMA * nas, int32 first, int32 last);
LEPT_DLL extern NUMA * numaMakeThresholdIndicator(NUMA * nas, float thresh, int32 type);
LEPT_DLL extern NUMA * numaUniformSampling(NUMA * nas, int32 nsamp);
LEPT_DLL extern NUMA * numaReverse(NUMA * nad, NUMA * nas);
LEPT_DLL extern NUMA * numaLowPassIntervals(NUMA * nas, float thresh, float maxn);
LEPT_DLL extern NUMA * numaThresholdEdges(NUMA * nas, float thresh1, float thresh2, float maxn);
LEPT_DLL extern int32 numaGetSpanValues(NUMA * na, int32 span, int32 * pstart, int32 * pend);
LEPT_DLL extern int32 numaGetEdgeValues(NUMA * na, int32 edge, int32 * pstart, int32 * pend, int32 * psign);
LEPT_DLL extern l_ok numaInterpolateEqxVal(float startx, float deltax, NUMA * nay, int32 type, float xval, float * pyval);
LEPT_DLL extern l_ok numaInterpolateArbxVal(NUMA * nax, NUMA * nay, int32 type, float xval, float * pyval);
LEPT_DLL extern l_ok numaInterpolateEqxInterval(float startx,
    float deltax,
    NUMA * nasy,
    int32 type,
    float x0,
    float x1,
    int32 npts,
    NUMA ** pnax,
    NUMA ** pnay);
LEPT_DLL extern l_ok numaInterpolateArbxInterval(NUMA * nax,
    NUMA * nay,
    int32 type,
    float x0,
    float x1,
    int32 npts,
    NUMA ** pnadx,
    NUMA ** pnady);
LEPT_DLL extern l_ok numaFitMax(NUMA * na, float * pmaxval, NUMA * naloc, float * pmaxloc);
LEPT_DLL extern l_ok numaDifferentiateInterval(NUMA * nax,
    NUMA * nay,
    float x0,
    float x1,
    int32 npts,
    NUMA ** pnadx,
    NUMA ** pnady);
LEPT_DLL extern l_ok numaIntegrateInterval(NUMA * nax, NUMA * nay, float x0, float x1, int32 npts, float * psum);
LEPT_DLL extern l_ok numaSortGeneral(NUMA * na, NUMA ** pnasort, NUMA ** pnaindex, NUMA ** pnainvert, int32 sortorder, int32 sorttype);
LEPT_DLL extern NUMA * numaSortAutoSelect(NUMA * nas, int32 sortorder);
LEPT_DLL extern NUMA * numaSortIndexAutoSelect(NUMA * nas, int32 sortorder);
LEPT_DLL extern int32 numaChooseSortType(NUMA * nas);
LEPT_DLL extern NUMA * numaSort(NUMA * naout, NUMA * nain, int32 sortorder);
LEPT_DLL extern NUMA * numaBinSort(NUMA * nas, int32 sortorder);
LEPT_DLL extern NUMA * numaGetSortIndex(NUMA * na, int32 sortorder);
LEPT_DLL extern NUMA * numaGetBinSortIndex(NUMA * nas, int32 sortorder);
LEPT_DLL extern NUMA * numaSortByIndex(NUMA * nas, NUMA * naindex);
LEPT_DLL extern int32 numaIsSorted(NUMA * nas, int32 sortorder, int32 * psorted);
LEPT_DLL extern l_ok numaSortPair(NUMA * nax, NUMA * nay, int32 sortorder, NUMA ** pnasx, NUMA ** pnasy);
LEPT_DLL extern NUMA * numaInvertMap(NUMA * nas);
LEPT_DLL extern l_ok numaAddSorted(NUMA * na, float val);
LEPT_DLL extern l_ok numaFindSortedLoc(NUMA * na, float val, int32 * pindex);
LEPT_DLL extern NUMA * numaPseudorandomSequence(int32 size, int32 seed);
LEPT_DLL extern NUMA * numaRandomPermutation(NUMA * nas, int32 seed);
LEPT_DLL extern l_ok numaGetRankValue(NUMA * na, float fract, NUMA * nasort, int32 usebins, float * pval);
LEPT_DLL extern l_ok numaGetMedian(NUMA * na, float * pval);
LEPT_DLL extern l_ok numaGetBinnedMedian(NUMA * na, int32 * pval);
LEPT_DLL extern l_ok numaGetMeanDevFromMedian(NUMA * na, float med, float * pdev);
LEPT_DLL extern l_ok numaGetMedianDevFromMedian(NUMA * na, float * pmed, float * pdev);
LEPT_DLL extern l_ok numaGetMode(NUMA * na, float * pval, int32 * pcount);
LEPT_DLL extern l_ok numaJoin(NUMA * nad, NUMA * nas, int32 istart, int32 iend);
LEPT_DLL extern l_ok numaaJoin(NUMAA * naad, NUMAA * naas, int32 istart, int32 iend);
LEPT_DLL extern NUMA * numaaFlattenToNuma(NUMAA * naa);
LEPT_DLL extern NUMA * numaErode(NUMA * nas, int32 size);
LEPT_DLL extern NUMA * numaDilate(NUMA * nas, int32 size);
LEPT_DLL extern NUMA * numaOpen(NUMA * nas, int32 size);
LEPT_DLL extern NUMA * numaClose(NUMA * nas, int32 size);
LEPT_DLL extern NUMA * numaTransform(NUMA * nas, float shift, float scale);
LEPT_DLL extern l_ok numaSimpleStats(NUMA * na, int32 first, int32 last, float * pmean, float * pvar, float * prvar);
LEPT_DLL extern l_ok numaWindowedStats(NUMA * nas, int32 wc, NUMA ** pnam, NUMA ** pnams, NUMA ** pnav, NUMA ** pnarv);
LEPT_DLL extern NUMA * numaWindowedMean(NUMA * nas, int32 wc);
LEPT_DLL extern NUMA * numaWindowedMeanSquare(NUMA * nas, int32 wc);
LEPT_DLL extern l_ok numaWindowedVariance(NUMA * nam, NUMA * nams, NUMA ** pnav, NUMA ** pnarv);
LEPT_DLL extern NUMA * numaWindowedMedian(NUMA * nas, int32 halfwin);
LEPT_DLL extern NUMA * numaConvertToInt(NUMA * nas);
LEPT_DLL extern NUMA * numaMakeHistogram(NUMA * na, int32 maxbins, int32 * pbinsize, int32 * pbinstart);
LEPT_DLL extern NUMA * numaMakeHistogramAuto(NUMA * na, int32 maxbins);
LEPT_DLL extern NUMA * numaMakeHistogramClipped(NUMA * na, float binsize, float maxsize);
LEPT_DLL extern NUMA * numaRebinHistogram(NUMA * nas, int32 newsize);
LEPT_DLL extern NUMA * numaNormalizeHistogram(NUMA * nas, float tsum);
LEPT_DLL extern l_ok numaGetStatsUsingHistogram(NUMA * na,
    int32 maxbins,
    float * pmin,
    float * pmax,
    float * pmean,
    float * pvariance,
    float * pmedian,
    float rank,
    float * prval,
    NUMA ** phisto);
LEPT_DLL extern l_ok numaGetHistogramStats(NUMA * nahisto,
    float startx,
    float deltax,
    float * pxmean,
    float * pxmedian,
    float * pxmode,
    float * pxvariance);
LEPT_DLL extern l_ok numaGetHistogramStatsOnInterval(NUMA * nahisto,
    float startx,
    float deltax,
    int32 ifirst,
    int32 ilast,
    float * pxmean,
    float * pxmedian,
    float * pxmode,
    float * pxvariance);
LEPT_DLL extern l_ok numaMakeRankFromHistogram(float startx, float deltax, NUMA * nasy, int32 npts, NUMA ** pnax, NUMA ** pnay);
LEPT_DLL extern l_ok numaHistogramGetRankFromVal(NUMA * na, float rval, float * prank);
LEPT_DLL extern l_ok numaHistogramGetValFromRank(NUMA * na, float rank, float * prval);
LEPT_DLL extern l_ok numaDiscretizeSortedInBins(NUMA * na, int32 nbins, NUMA ** pnabinval);
LEPT_DLL extern l_ok numaDiscretizeHistoInBins(NUMA * na, int32 nbins, NUMA ** pnabinval, NUMA ** pnarank);
LEPT_DLL extern l_ok numaGetRankBinValues(NUMA * na, int32 nbins, NUMA ** pnam);
LEPT_DLL extern NUMA * numaGetUniformBinSizes(int32 ntotal, int32 nbins);
LEPT_DLL extern l_ok numaSplitDistribution(NUMA * na,
    float scorefract,
    int32 * psplitindex,
    float * pave1,
    float * pave2,
    float * pnum1,
    float * pnum2,
    NUMA ** pnascore);
LEPT_DLL extern l_ok grayHistogramsToEMD(NUMAA * naa1, NUMAA * naa2, NUMA ** pnad);
LEPT_DLL extern l_ok numaEarthMoverDistance(NUMA * na1, NUMA * na2, float * pdist);
LEPT_DLL extern l_ok grayInterHistogramStats(NUMAA * naa, int32 wc, NUMA ** pnam, NUMA ** pnams, NUMA ** pnav, NUMA ** pnarv);
LEPT_DLL extern NUMA * numaFindPeaks(NUMA * nas, int32 nmax, float fract1, float fract2);
LEPT_DLL extern NUMA * numaFindExtrema(NUMA * nas, float delta, NUMA ** pnav);
LEPT_DLL extern l_ok numaFindLocForThreshold(NUMA * na, int32 skip, int32 * pthresh, float * pfract);
LEPT_DLL extern l_ok numaCountReversals(NUMA * nas, float minreversal, int32 * pnr, float * prd);
LEPT_DLL extern l_ok numaSelectCrossingThreshold(NUMA * nax, NUMA * nay, float estthresh, float * pbestthresh);
LEPT_DLL extern NUMA * numaCrossingsByThreshold(NUMA * nax, NUMA * nay, float thresh);
LEPT_DLL extern NUMA * numaCrossingsByPeaks(NUMA * nax, NUMA * nay, float delta);
LEPT_DLL extern l_ok numaEvalBestHaarParameters(NUMA * nas,
    float relweight,
    int32 nwidth,
    int32 nshift,
    float minwidth,
    float maxwidth,
    float * pbestwidth,
    float * pbestshift,
    float * pbestscore);
LEPT_DLL extern l_ok numaEvalHaarSum(NUMA * nas, float width, float shift, float relweight, float * pscore);
LEPT_DLL extern NUMA * genConstrainedNumaInRange(int32 first, int32 last, int32 nmax, int32 use_pairs);
LEPT_DLL extern l_ok pixGetRegionsBinary(PIX * pixs, PIX ** ppixhm, PIX ** ppixtm, PIX ** ppixtb, PIXA * pixadb);
LEPT_DLL extern PIX * pixGenHalftoneMask(PIX * pixs, PIX ** ppixtext, int32 * phtfound, int32 debug);
LEPT_DLL extern PIX * pixGenerateHalftoneMask(PIX * pixs, PIX ** ppixtext, int32 * phtfound, PIXA * pixadb);
LEPT_DLL extern PIX * pixGenTextlineMask(PIX * pixs, PIX ** ppixvws, int32 * ptlfound, PIXA * pixadb);
LEPT_DLL extern PIX * pixGenTextblockMask(PIX * pixs, PIX * pixvws, PIXA * pixadb);
LEPT_DLL extern BOX * pixFindPageForeground(PIX * pixs,
    int32 threshold,
    int32 mindist,
    int32 erasedist,
    int32 showmorph,
    PIXAC * pixac);
LEPT_DLL extern l_ok pixSplitIntoCharacters(PIX * pixs, int32 minw, int32 minh, BOXA ** pboxa, PIXA ** ppixa, PIX ** ppixdebug);
LEPT_DLL extern BOXA * pixSplitComponentWithProfile(PIX * pixs, int32 delta, int32 mindel, PIX ** ppixdebug);
LEPT_DLL extern PIXA * pixExtractTextlines(PIX * pixs,
    int32 maxw,
    int32 maxh,
    int32 minw,
    int32 minh,
    int32 adjw,
    int32 adjh,
    PIXA * pixadb);
LEPT_DLL extern PIXA * pixExtractRawTextlines(PIX * pixs, int32 maxw, int32 maxh, int32 adjw, int32 adjh, PIXA * pixadb);
LEPT_DLL extern l_ok pixCountTextColumns(PIX * pixs,
    float deltafract,
    float peakfract,
    float clipfract,
    int32 * pncols,
    PIXA * pixadb);
LEPT_DLL extern l_ok pixDecideIfText(PIX * pixs, BOX * box, int32 * pistext, PIXA * pixadb);
LEPT_DLL extern l_ok pixFindThreshFgExtent(PIX * pixs, int32 thresh, int32 * ptop, int32 * pbot);
LEPT_DLL extern l_ok pixDecideIfTable(PIX * pixs, BOX * box, int32 orient, int32 * pscore, PIXA * pixadb);
LEPT_DLL extern PIX * pixPrepare1bpp(PIX * pixs, BOX * box, float cropfract, int32 outres);
LEPT_DLL extern l_ok pixEstimateBackground(PIX * pixs, int32 darkthresh, float edgecrop, int32 * pbg);
LEPT_DLL extern l_ok pixFindLargeRectangles(PIX * pixs, int32 polarity, int32 nrect, BOXA ** pboxa, PIX ** ppixdb);
LEPT_DLL extern l_ok pixFindLargestRectangle(PIX * pixs, int32 polarity, BOX ** pbox, PIX ** ppixdb);
LEPT_DLL extern BOX * pixFindRectangleInCC(PIX * pixs, BOX * boxs, float fract, int32 dir, int32 select, int32 debug);
LEPT_DLL extern PIX * pixAutoPhotoinvert(PIX * pixs, int32 thresh, PIX ** ppixm, PIXA * pixadb);
LEPT_DLL extern l_ok pixSetSelectCmap(PIX * pixs, BOX * box, int32 sindex, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixColorGrayRegionsCmap(PIX * pixs, BOXA * boxa, int32 type, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixColorGrayCmap(PIX * pixs, BOX * box, int32 type, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixColorGrayMaskedCmap(PIX * pixs, PIX * pixm, int32 type, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok addColorizedGrayToCmap(PIXCMAP * cmap, int32 type, int32 rval, int32 gval, int32 bval, NUMA ** pna);
LEPT_DLL extern l_ok pixSetSelectMaskedCmap(PIX * pixs,
    PIX * pixm,
    int32 x,
    int32 y,
    int32 sindex,
    int32 rval,
    int32 gval,
    int32 bval);
LEPT_DLL extern l_ok pixSetMaskedCmap(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern char * parseForProtos(const char * filein, const char * prestring);
LEPT_DLL extern l_ok partifyFiles(const char * dirname, const char * substr, int32 nparts, const char * outroot, const char * debugfile);
LEPT_DLL extern l_ok partifyPixac(PIXAC * pixac, int32 nparts, const char * outroot, PIXA * pixadb);
LEPT_DLL extern BOXA * boxaGetWhiteblocks(BOXA * boxas,
    BOX * box,
    int32 sortflag,
    int32 maxboxes,
    float maxoverlap,
    int32 maxperim,
    float fract,
    int32 maxpops);
LEPT_DLL extern BOXA * boxaPruneSortedOnOverlap(BOXA * boxas, float maxoverlap);
LEPT_DLL extern l_ok convertFilesToPdf(const char * dirname,
    const char * substr,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok saConvertFilesToPdf(SARRAY * sa,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok saConvertFilesToPdfData(SARRAY * sa,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    uint8 ** pdata,
    size_t * pnbytes);
LEPT_DLL extern l_ok selectDefaultPdfEncoding(PIX * pix, int32 * ptype);
LEPT_DLL extern l_ok convertUnscaledFilesToPdf(const char * dirname, const char * substr, const char * title, const char * fileout);
LEPT_DLL extern l_ok saConvertUnscaledFilesToPdf(SARRAY * sa, const char * title, const char * fileout);
LEPT_DLL extern l_ok saConvertUnscaledFilesToPdfData(SARRAY * sa, const char * title, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok convertUnscaledToPdfData(const char * fname, const char * title, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok pixaConvertToPdf(PIXA * pixa,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok pixaConvertToPdfData(PIXA * pixa,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    uint8 ** pdata,
    size_t * pnbytes);
LEPT_DLL extern l_ok convertToPdf(const char * filein,
    int32 type,
    int32 quality,
    const char * fileout,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok convertImageDataToPdf(uint8 * imdata,
    size_t size,
    int32 type,
    int32 quality,
    const char * fileout,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok convertToPdfData(const char * filein,
    int32 type,
    int32 quality,
    uint8 ** pdata,
    size_t * pnbytes,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok convertImageDataToPdfData(uint8 * imdata,
    size_t size,
    int32 type,
    int32 quality,
    uint8 ** pdata,
    size_t * pnbytes,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok pixConvertToPdf(PIX * pix,
    int32 type,
    int32 quality,
    const char * fileout,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok pixWriteStreamPdf(FILE * fp, PIX * pix, int32 res, const char * title);
LEPT_DLL extern l_ok pixWriteMemPdf(uint8 ** pdata, size_t * pnbytes, PIX * pix, int32 res, const char * title);
LEPT_DLL extern l_ok convertSegmentedFilesToPdf(const char * dirname,
    const char * substr,
    int32 res,
    int32 type,
    int32 thresh,
    BOXAA * baa,
    int32 quality,
    float scalefactor,
    const char * title,
    const char * fileout);
LEPT_DLL extern BOXAA * convertNumberedMasksToBoxaa(const char * dirname, const char * substr, int32 numpre, int32 numpost);
LEPT_DLL extern l_ok convertToPdfSegmented(const char * filein,
    int32 res,
    int32 type,
    int32 thresh,
    BOXA * boxa,
    int32 quality,
    float scalefactor,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok pixConvertToPdfSegmented(PIX * pixs,
    int32 res,
    int32 type,
    int32 thresh,
    BOXA * boxa,
    int32 quality,
    float scalefactor,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok convertToPdfDataSegmented(const char * filein,
    int32 res,
    int32 type,
    int32 thresh,
    BOXA * boxa,
    int32 quality,
    float scalefactor,
    const char * title,
    uint8 ** pdata,
    size_t * pnbytes);
LEPT_DLL extern l_ok pixConvertToPdfDataSegmented(PIX * pixs,
    int32 res,
    int32 type,
    int32 thresh,
    BOXA * boxa,
    int32 quality,
    float scalefactor,
    const char * title,
    uint8 ** pdata,
    size_t * pnbytes);
LEPT_DLL extern l_ok concatenatePdf(const char * dirname, const char * substr, const char * fileout);
LEPT_DLL extern l_ok saConcatenatePdf(SARRAY * sa, const char * fileout);
LEPT_DLL extern l_ok ptraConcatenatePdf(L_PTRA * pa, const char * fileout);
LEPT_DLL extern l_ok concatenatePdfToData(const char * dirname, const char * substr, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok saConcatenatePdfToData(SARRAY * sa, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok pixConvertToPdfData(PIX * pix,
    int32 type,
    int32 quality,
    uint8 ** pdata,
    size_t * pnbytes,
    int32 x,
    int32 y,
    int32 res,
    const char * title,
    L_PDF_DATA ** plpd,
    int32 position);
LEPT_DLL extern l_ok ptraConcatenatePdfToData(L_PTRA * pa_data, SARRAY * sa, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok convertTiffMultipageToPdf(const char * filein, const char * fileout);
LEPT_DLL extern l_ok l_generateCIDataForPdf(const char * fname, PIX * pix, int32 quality, L_COMP_DATA ** pcid);
LEPT_DLL extern l_ok l_generateCIData(const char * fname, int32 type, int32 quality, int32 ascii85, L_COMP_DATA ** pcid);
LEPT_DLL extern L_COMP_DATA * l_generateFlateDataPdf(const char * fname, PIX * pixs);
LEPT_DLL extern L_COMP_DATA * l_generateJpegData(const char * fname, int32 ascii85flag);
LEPT_DLL extern L_COMP_DATA * l_generateJpegDataMem(uint8 * data, size_t nbytes, int32 ascii85flag);
LEPT_DLL extern L_COMP_DATA * l_generateG4Data(const char * fname, int32 ascii85flag);
LEPT_DLL extern l_ok pixGenerateCIData(PIX * pixs, int32 type, int32 quality, int32 ascii85, L_COMP_DATA ** pcid);
LEPT_DLL extern L_COMP_DATA * l_generateFlateData(const char * fname, int32 ascii85flag);
LEPT_DLL extern l_ok cidConvertToPdfData(L_COMP_DATA * cid, const char * title, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern void l_CIDataDestroy(L_COMP_DATA ** pcid);
LEPT_DLL extern void l_pdfSetG4ImageMask(int32 flag);
LEPT_DLL extern void l_pdfSetDateAndVersion(int32 flag);
LEPT_DLL extern void setPixMemoryManager(alloc_fn allocator, dealloc_fn deallocator);
LEPT_DLL extern PIX * pixCreate(int32 width, int32 height, int32 depth);
LEPT_DLL extern PIX * pixCreateNoInit(int32 width, int32 height, int32 depth);
LEPT_DLL extern PIX * pixCreateTemplate(const PIX * pixs);
LEPT_DLL extern PIX * pixCreateTemplateNoInit(const PIX * pixs);
LEPT_DLL extern PIX * pixCreateWithCmap(int32 width, int32 height, int32 depth, int32 initcolor);
LEPT_DLL extern PIX * pixCreateHeader(int32 width, int32 height, int32 depth);
LEPT_DLL extern PIX * pixClone(PIX * pixs);
LEPT_DLL extern void pixDestroy(PIX ** ppix);
LEPT_DLL extern PIX * pixCopy(PIX * pixd, const PIX * pixs);
LEPT_DLL extern l_ok pixResizeImageData(PIX * pixd, const PIX * pixs);
LEPT_DLL extern l_ok pixCopyColormap(PIX * pixd, const PIX * pixs);
LEPT_DLL extern l_ok pixTransferAllData(PIX * pixd, PIX ** ppixs, int32 copytext, int32 copyformat);
LEPT_DLL extern l_ok pixSwapAndDestroy(PIX ** ppixd, PIX ** ppixs);
LEPT_DLL extern int32 pixGetWidth(const PIX * pix);
LEPT_DLL extern int32 pixSetWidth(PIX * pix, int32 width);
LEPT_DLL extern int32 pixGetHeight(const PIX * pix);
LEPT_DLL extern int32 pixSetHeight(PIX * pix, int32 height);
LEPT_DLL extern int32 FASTCALL pixGetDepth(const PIX * pix);
LEPT_DLL extern int32 pixSetDepth(PIX * pix, int32 depth);
LEPT_DLL extern l_ok pixGetDimensions(const PIX * pix, int32 * pw, int32 * ph, int32 * pd);
LEPT_DLL extern l_ok pixSetDimensions(PIX * pix, int32 w, int32 h, int32 d);
LEPT_DLL extern l_ok pixCopyDimensions(PIX * pixd, const PIX * pixs);
LEPT_DLL extern int32 pixGetSpp(const PIX * pix);
LEPT_DLL extern int32 pixSetSpp(PIX * pix, int32 spp);
LEPT_DLL extern l_ok pixCopySpp(PIX * pixd, const PIX * pixs);
LEPT_DLL extern int32 pixGetWpl(const PIX * pix);
LEPT_DLL extern int32 pixSetWpl(PIX * pix, int32 wpl);
LEPT_DLL extern int32 pixGetRefcount(const PIX * pix);
LEPT_DLL extern int32 pixChangeRefcount(PIX * pix, int32 delta);
LEPT_DLL extern int32 pixGetXRes(const PIX * pix);
LEPT_DLL extern int32 pixSetXRes(PIX * pix, int32 res);
LEPT_DLL extern int32 pixGetYRes(const PIX * pix);
LEPT_DLL extern int32 pixSetYRes(PIX * pix, int32 res);
LEPT_DLL extern l_ok pixGetResolution(const PIX * pix, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok pixSetResolution(PIX * pix, int32 xres, int32 yres);
LEPT_DLL extern int32 pixCopyResolution(PIX * pixd, const PIX * pixs);
LEPT_DLL extern int32 pixScaleResolution(PIX * pix, float xscale, float yscale);
LEPT_DLL extern int32 pixGetInputFormat(const PIX * pix);
LEPT_DLL extern int32 pixSetInputFormat(PIX * pix, int32 informat);
LEPT_DLL extern int32 pixCopyInputFormat(PIX * pixd, const PIX * pixs);
LEPT_DLL extern int32 pixSetSpecial(PIX * pix, int32 special);
LEPT_DLL extern char * pixGetText(PIX * pix);
LEPT_DLL extern l_ok pixSetText(PIX * pix, const char * textstring);
LEPT_DLL extern l_ok pixAddText(PIX * pix, const char * textstring);
LEPT_DLL extern int32 pixCopyText(PIX * pixd, const PIX * pixs);
LEPT_DLL extern uint8 * pixGetTextCompNew(PIX * pix, size_t * psize);
LEPT_DLL extern l_ok pixSetTextCompNew(PIX * pix, const uint8 * data, size_t size);
LEPT_DLL extern PIXCMAP * FASTCALL pixGetColormap(PIX * pix);
LEPT_DLL extern l_ok pixSetColormap(PIX * pix, PIXCMAP * colormap);
LEPT_DLL extern l_ok pixDestroyColormap(PIX * pix);
LEPT_DLL extern uint32 * pixGetData(PIX * pix);
LEPT_DLL extern int32 pixSetData(PIX * pix, uint32 * data);
LEPT_DLL extern uint32 * pixExtractData(PIX * pixs);
LEPT_DLL extern int32 pixFreeData(PIX * pix);
LEPT_DLL extern void ** pixGetLinePtrs(PIX * pix, int32 * psize);
LEPT_DLL extern int32 pixSizesEqual(const PIX * pix1, const PIX * pix2);
LEPT_DLL extern l_ok pixMaxAspectRatio(PIX * pixs, float * pratio);
LEPT_DLL extern l_ok pixPrintStreamInfo(FILE * fp, const PIX * pix, const char * text);
LEPT_DLL extern l_ok pixGetPixel(PIX * pix, int32 x, int32 y, uint32 * pval);
LEPT_DLL extern l_ok pixSetPixel(PIX * pix, int32 x, int32 y, uint32 val);
LEPT_DLL extern l_ok pixGetRGBPixel(PIX * pix, int32 x, int32 y, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern l_ok pixSetRGBPixel(PIX * pix, int32 x, int32 y, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixSetCmapPixel(PIX * pix, int32 x, int32 y, int32 rval, int32 gval, int32 bval);
LEPT_DLL extern l_ok pixGetRandomPixel(PIX * pix, uint32 * pval, int32 * px, int32 * py);
LEPT_DLL extern l_ok pixClearPixel(PIX * pix, int32 x, int32 y);
LEPT_DLL extern l_ok pixFlipPixel(PIX * pix, int32 x, int32 y);
LEPT_DLL extern void setPixelLow(uint32 * line, int32 x, int32 depth, uint32 val);
LEPT_DLL extern l_ok pixGetBlackOrWhiteVal(PIX * pixs, int32 op, uint32 * pval);
LEPT_DLL extern l_ok pixClearAll(PIX * pix);
LEPT_DLL extern l_ok pixSetAll(PIX * pix);
LEPT_DLL extern l_ok pixSetAllGray(PIX * pix, int32 grayval);
LEPT_DLL extern l_ok pixSetAllArbitrary(PIX * pix, uint32 val);
LEPT_DLL extern l_ok pixSetBlackOrWhite(PIX * pixs, int32 op);
LEPT_DLL extern l_ok pixSetComponentArbitrary(PIX * pix, int32 comp, int32 val);
LEPT_DLL extern l_ok pixClearInRect(PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixSetInRect(PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixSetInRectArbitrary(PIX * pix, BOX * box, uint32 val);
LEPT_DLL extern l_ok pixBlendInRect(PIX * pixs, BOX * box, uint32 val, float fract);
LEPT_DLL extern l_ok pixSetPadBits(PIX * pix, int32 val);
LEPT_DLL extern l_ok pixSetPadBitsBand(PIX * pix, int32 by, int32 bh, int32 val);
LEPT_DLL extern l_ok pixSetOrClearBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot, int32 op);
LEPT_DLL extern l_ok pixSetBorderVal(PIX * pixs, int32 left, int32 right, int32 top, int32 bot, uint32 val);
LEPT_DLL extern l_ok pixSetBorderRingVal(PIX * pixs, int32 dist, uint32 val);
LEPT_DLL extern l_ok pixSetMirroredBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixCopyBorder(PIX * pixd, PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixAddBorder(PIX * pixs, int32 npix, uint32 val);
LEPT_DLL extern PIX * pixAddBlackOrWhiteBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot, int32 op);
LEPT_DLL extern PIX * pixAddBorderGeneral(PIX * pixs, int32 left, int32 right, int32 top, int32 bot, uint32 val);
LEPT_DLL extern PIX * pixRemoveBorder(PIX * pixs, int32 npix);
LEPT_DLL extern PIX * pixRemoveBorderGeneral(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixRemoveBorderToSize(PIX * pixs, int32 wd, int32 hd);
LEPT_DLL extern PIX * pixAddMirroredBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixAddRepeatedBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixAddMixedBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern PIX * pixAddContinuedBorder(PIX * pixs, int32 left, int32 right, int32 top, int32 bot);
LEPT_DLL extern l_ok pixShiftAndTransferAlpha(PIX * pixd, PIX * pixs, float shiftx, float shifty);
LEPT_DLL extern PIX * pixDisplayLayersRGBA(PIX * pixs, uint32 val, int32 maxw);
LEPT_DLL extern PIX * pixCreateRGBImage(PIX * pixr, PIX * pixg, PIX * pixb);
LEPT_DLL extern PIX * pixGetRGBComponent(PIX * pixs, int32 comp);
LEPT_DLL extern l_ok pixSetRGBComponent(PIX * pixd, PIX * pixs, int32 comp);
LEPT_DLL extern PIX * pixGetRGBComponentCmap(PIX * pixs, int32 comp);
LEPT_DLL extern l_ok pixCopyRGBComponent(PIX * pixd, PIX * pixs, int32 comp);
LEPT_DLL extern l_ok composeRGBPixel(int32 rval, int32 gval, int32 bval, uint32 * ppixel);
LEPT_DLL extern l_ok composeRGBAPixel(int32 rval, int32 gval, int32 bval, int32 aval, uint32 * ppixel);
LEPT_DLL extern void extractRGBValues(uint32 pixel, int32 * prval, int32 * pgval, int32 * pbval);
LEPT_DLL extern void extractRGBAValues(uint32 pixel, int32 * prval, int32 * pgval, int32 * pbval, int32 * paval);
LEPT_DLL extern int32 extractMinMaxComponent(uint32 pixel, int32 type);
LEPT_DLL extern l_ok pixGetRGBLine(PIX * pixs, int32 row, uint8 * bufr, uint8 * bufg, uint8 * bufb);
LEPT_DLL extern l_ok setLineDataVal(uint32 * line, int32 j, int32 d, uint32 val);
LEPT_DLL extern PIX * pixEndianByteSwapNew(PIX * pixs);
LEPT_DLL extern l_ok pixEndianByteSwap(PIX * pixs);
LEPT_DLL extern int32 lineEndianByteSwap(uint32 * datad, uint32 * datas, int32 wpl);
LEPT_DLL extern PIX * pixEndianTwoByteSwapNew(PIX * pixs);
LEPT_DLL extern l_ok pixEndianTwoByteSwap(PIX * pixs);
LEPT_DLL extern l_ok pixGetRasterData(PIX * pixs, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok pixInferResolution(PIX * pix, float longside, int32 * pres);
LEPT_DLL extern l_ok pixAlphaIsOpaque(PIX * pix, int32 * popaque);
LEPT_DLL extern uint8 ** pixSetupByteProcessing(PIX * pix, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok pixCleanupByteProcessing(PIX * pix, uint8 ** lineptrs);
LEPT_DLL extern void l_setAlphaMaskBorder(float val1, float val2);
LEPT_DLL extern l_ok pixSetMasked(PIX * pixd, PIX * pixm, uint32 val);
LEPT_DLL extern l_ok pixSetMaskedGeneral(PIX * pixd, PIX * pixm, uint32 val, int32 x, int32 y);
LEPT_DLL extern l_ok pixCombineMasked(PIX * pixd, PIX * pixs, PIX * pixm);
LEPT_DLL extern l_ok pixCombineMaskedGeneral(PIX * pixd, PIX * pixs, PIX * pixm, int32 x, int32 y);
LEPT_DLL extern l_ok pixPaintThroughMask(PIX * pixd, PIX * pixm, int32 x, int32 y, uint32 val);
LEPT_DLL extern PIX * pixCopyWithBoxa(PIX * pixs, BOXA * boxa, int32 background);
LEPT_DLL extern l_ok pixPaintSelfThroughMask(PIX * pixd, PIX * pixm, int32 x, int32 y, int32 searchdir, int32 mindist, int32 tilesize, int32 ntiles, int32 distblend);
LEPT_DLL extern PIX * pixMakeMaskFromVal(PIX * pixs, int32 val);
LEPT_DLL extern PIX * pixMakeMaskFromLUT(PIX * pixs, int32 * tab);
LEPT_DLL extern PIX * pixMakeArbMaskFromRGB(PIX * pixs, float rc, float gc, float bc, float thresh);
LEPT_DLL extern PIX * pixSetUnderTransparency(PIX * pixs, uint32 val, int32 debug);
LEPT_DLL extern PIX * pixMakeAlphaFromMask(PIX * pixs, int32 dist, BOX ** pbox);
LEPT_DLL extern l_ok pixGetColorNearMaskBoundary(PIX * pixs, PIX * pixm, BOX * box, int32 dist, uint32 * pval, int32 debug);
LEPT_DLL extern PIX * pixDisplaySelectedPixels(PIX * pixs, PIX * pixm, SEL * sel, uint32 val);
LEPT_DLL extern PIX * pixInvert(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixOr(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixAnd(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixXor(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixSubtract(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern l_ok pixZero(PIX * pix, int32 * pempty);
LEPT_DLL extern l_ok pixForegroundFraction(PIX * pix, float * pfract);
LEPT_DLL extern NUMA * pixaCountPixels(PIXA * pixa);
LEPT_DLL extern l_ok pixCountPixels(PIX * pixs, int32 * pcount, int32 * tab8);
LEPT_DLL extern l_ok pixCountPixelsInRect(PIX * pixs, BOX * box, int32 * pcount, int32 * tab8);
LEPT_DLL extern NUMA * pixCountByRow(PIX * pix, BOX * box);
LEPT_DLL extern NUMA * pixCountByColumn(PIX * pix, BOX * box);
LEPT_DLL extern NUMA * pixCountPixelsByRow(PIX * pix, int32 * tab8);
LEPT_DLL extern NUMA * pixCountPixelsByColumn(PIX * pix);
LEPT_DLL extern l_ok pixCountPixelsInRow(PIX * pix, int32 row, int32 * pcount, int32 * tab8);
LEPT_DLL extern NUMA * pixGetMomentByColumn(PIX * pix, int32 order);
LEPT_DLL extern l_ok pixThresholdPixelSum(PIX * pix, int32 thresh, int32 * pabove, int32 * tab8);
LEPT_DLL extern int32 * makePixelSumTab8(void);
LEPT_DLL extern int32 * makePixelCentroidTab8(void);
LEPT_DLL extern NUMA * pixAverageByRow(PIX * pix, BOX * box, int32 type);
LEPT_DLL extern NUMA * pixAverageByColumn(PIX * pix, BOX * box, int32 type);
LEPT_DLL extern l_ok pixAverageInRect(PIX * pixs, PIX * pixm, BOX * box, int32 minval, int32 maxval, int32 subsamp, float * pave);
LEPT_DLL extern l_ok pixAverageInRectRGB(PIX * pixs, PIX * pixm, BOX * box, int32 subsamp, uint32 * pave);
LEPT_DLL extern NUMA * pixVarianceByRow(PIX * pix, BOX * box);
LEPT_DLL extern NUMA * pixVarianceByColumn(PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixVarianceInRect(PIX * pix, BOX * box, float * prootvar);
LEPT_DLL extern NUMA * pixAbsDiffByRow(PIX * pix, BOX * box);
LEPT_DLL extern NUMA * pixAbsDiffByColumn(PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixAbsDiffInRect(PIX * pix, BOX * box, int32 dir, float * pabsdiff);
LEPT_DLL extern l_ok pixAbsDiffOnLine(PIX * pix, int32 x1, int32 y1, int32 x2, int32 y2, float * pabsdiff);
LEPT_DLL extern int32 pixCountArbInRect(PIX * pixs, BOX * box, int32 val, int32 factor, int32 * pcount);
LEPT_DLL extern PIX * pixMirroredTiling(PIX * pixs, int32 w, int32 h);
LEPT_DLL extern l_ok pixFindRepCloseTile(PIX * pixs, BOX * box, int32 searchdir, int32 mindist, int32 tsize, int32 ntiles, BOX ** pboxtile, int32 debug);
LEPT_DLL extern NUMA * pixGetGrayHistogram(PIX * pixs, int32 factor);
LEPT_DLL extern NUMA * pixGetGrayHistogramMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor);
LEPT_DLL extern NUMA * pixGetGrayHistogramInRect(PIX * pixs, BOX * box, int32 factor);
LEPT_DLL extern NUMAA * pixGetGrayHistogramTiled(PIX * pixs, int32 factor, int32 nx, int32 ny);
LEPT_DLL extern l_ok pixGetColorHistogram(PIX * pixs, int32 factor, NUMA ** pnar, NUMA ** pnag, NUMA ** pnab);
LEPT_DLL extern l_ok pixGetColorHistogramMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, NUMA ** pnar, NUMA ** pnag, NUMA ** pnab);
LEPT_DLL extern NUMA * pixGetCmapHistogram(PIX * pixs, int32 factor);
LEPT_DLL extern NUMA * pixGetCmapHistogramMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor);
LEPT_DLL extern NUMA * pixGetCmapHistogramInRect(PIX * pixs, BOX * box, int32 factor);
LEPT_DLL extern l_ok pixCountRGBColorsByHash(PIX * pixs, int32 * pncolors);
LEPT_DLL extern l_ok pixCountRGBColors(PIX * pixs, int32 factor, int32 * pncolors);
LEPT_DLL extern L_AMAP * pixGetColorAmapHistogram(PIX * pixs, int32 factor);
LEPT_DLL extern int32 amapGetCountForColor(L_AMAP * amap, uint32 val);
LEPT_DLL extern l_ok pixGetRankValue(PIX * pixs, int32 factor, float rank, uint32 * pvalue);
LEPT_DLL extern l_ok pixGetRankValueMaskedRGB(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, float rank, float * prval, float * pgval, float * pbval);
LEPT_DLL extern l_ok pixGetRankValueMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, float rank, float * pval, NUMA ** pna);
LEPT_DLL extern l_ok pixGetPixelAverage(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, uint32 * pval);
LEPT_DLL extern l_ok pixGetPixelStats(PIX * pixs, int32 factor, int32 type, uint32 * pvalue);
LEPT_DLL extern l_ok pixGetAverageMaskedRGB(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, int32 type, float * prval, float * pgval, float * pbval);
LEPT_DLL extern l_ok pixGetAverageMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, int32 factor, int32 type, float * pval);
LEPT_DLL extern l_ok pixGetAverageTiledRGB(PIX * pixs, int32 sx, int32 sy, int32 type, PIX ** ppixr, PIX ** ppixg, PIX ** ppixb);
LEPT_DLL extern PIX * pixGetAverageTiled(PIX * pixs, int32 sx, int32 sy, int32 type);
LEPT_DLL extern int32 pixRowStats(PIX * pixs, BOX * box, NUMA ** pnamean, NUMA ** pnamedian, NUMA ** pnamode, NUMA ** pnamodecount, NUMA ** pnavar, NUMA ** pnarootvar);
LEPT_DLL extern int32 pixColumnStats(PIX * pixs, BOX * box, NUMA ** pnamean, NUMA ** pnamedian, NUMA ** pnamode, NUMA ** pnamodecount, NUMA ** pnavar, NUMA ** pnarootvar);
LEPT_DLL extern l_ok pixGetRangeValues(PIX * pixs, int32 factor, int32 color, int32 * pminval, int32 * pmaxval);
LEPT_DLL extern l_ok pixGetExtremeValue(PIX * pixs, int32 factor, int32 type, int32 * prval, int32 * pgval, int32 * pbval, int32 * pgrayval);
LEPT_DLL extern l_ok pixGetMaxValueInRect(PIX * pixs, BOX * box, uint32 * pmaxval, int32 * pxmax, int32 * pymax);
LEPT_DLL extern l_ok pixGetMaxColorIndex(PIX * pixs, int32 * pmaxindex);
LEPT_DLL extern l_ok pixGetBinnedComponentRange(PIX * pixs, int32 nbins, int32 factor, int32 color, int32 * pminval, int32 * pmaxval, uint32 ** pcarray, int32 fontsize);
LEPT_DLL extern l_ok pixGetRankColorArray(PIX * pixs, int32 nbins, int32 type, int32 factor, uint32 ** pcarray, PIXA * pixadb, int32 fontsize);
LEPT_DLL extern l_ok pixGetBinnedColor(PIX * pixs, PIX * pixg, int32 factor, int32 nbins, uint32 ** pcarray, PIXA * pixadb);
LEPT_DLL extern PIX * pixDisplayColorArray(uint32 * carray, int32 ncolors, int32 side, int32 ncols, int32 fontsize);
LEPT_DLL extern PIX * pixRankBinByStrip(PIX * pixs, int32 direction, int32 size, int32 nbins, int32 type);
LEPT_DLL extern PIX * pixaGetAlignedStats(PIXA * pixa, int32 type, int32 nbins, int32 thresh);
LEPT_DLL extern l_ok pixaExtractColumnFromEachPix(PIXA * pixa, int32 col, PIX * pixd);
LEPT_DLL extern l_ok pixGetRowStats(PIX * pixs, int32 type, int32 nbins, int32 thresh, float * colvect);
LEPT_DLL extern l_ok pixGetColumnStats(PIX * pixs, int32 type, int32 nbins, int32 thresh, float * rowvect);
LEPT_DLL extern l_ok pixSetPixelColumn(PIX * pix, int32 col, float * colvect);
LEPT_DLL extern l_ok pixThresholdForFgBg(PIX * pixs, int32 factor, int32 thresh, int32 * pfgval, int32 * pbgval);
LEPT_DLL extern l_ok pixSplitDistributionFgBg(PIX * pixs, float scorefract, int32 factor, int32 * pthresh, int32 * pfgval, int32 * pbgval, PIX ** ppixdb);
LEPT_DLL extern l_ok pixaFindDimensions(PIXA * pixa, NUMA ** pnaw, NUMA ** pnah);
LEPT_DLL extern l_ok pixFindAreaPerimRatio(PIX * pixs, int32 * tab, float * pfract);
LEPT_DLL extern NUMA * pixaFindPerimToAreaRatio(PIXA * pixa);
LEPT_DLL extern l_ok pixFindPerimToAreaRatio(PIX * pixs, int32 * tab, float * pfract);
LEPT_DLL extern NUMA * pixaFindPerimSizeRatio(PIXA * pixa);
LEPT_DLL extern l_ok pixFindPerimSizeRatio(PIX * pixs, int32 * tab, float * pratio);
LEPT_DLL extern NUMA * pixaFindAreaFraction(PIXA * pixa);
LEPT_DLL extern l_ok pixFindAreaFraction(PIX * pixs, int32 * tab, float * pfract);
LEPT_DLL extern NUMA * pixaFindAreaFractionMasked(PIXA * pixa, PIX * pixm, int32 debug);
LEPT_DLL extern l_ok pixFindAreaFractionMasked(PIX * pixs, BOX * box, PIX * pixm, int32 * tab, float * pfract);
LEPT_DLL extern NUMA * pixaFindWidthHeightRatio(PIXA * pixa);
LEPT_DLL extern NUMA * pixaFindWidthHeightProduct(PIXA * pixa);
LEPT_DLL extern l_ok pixFindOverlapFraction(PIX * pixs1, PIX * pixs2, int32 x2, int32 y2, int32 * tab, float * pratio, int32 * pnoverlap);
LEPT_DLL extern BOXA * pixFindRectangleComps(PIX * pixs, int32 dist, int32 minw, int32 minh);
LEPT_DLL extern l_ok pixConformsToRectangle(PIX * pixs, BOX * box, int32 dist, int32 * pconforms);
LEPT_DLL extern PIXA * pixClipRectangles(PIX * pixs, BOXA * boxa);
LEPT_DLL extern PIX * pixClipRectangle(PIX * pixs, BOX * box, BOX ** pboxc);
LEPT_DLL extern PIX * pixClipRectangleWithBorder(PIX * pixs, BOX * box, int32 maxbord, BOX ** pboxn);
LEPT_DLL extern PIX * pixClipMasked(PIX * pixs, PIX * pixm, int32 x, int32 y, uint32 outval);
LEPT_DLL extern l_ok pixCropToMatch(PIX * pixs1, PIX * pixs2, PIX ** ppixd1, PIX ** ppixd2);
LEPT_DLL extern PIX * pixCropToSize(PIX * pixs, int32 w, int32 h);
LEPT_DLL extern PIX * pixResizeToMatch(PIX * pixs, PIX * pixt, int32 w, int32 h);
LEPT_DLL extern PIX * pixSelectComponentBySize(PIX * pixs, int32 rankorder, int32 type, int32 connectivity, BOX ** pbox);
LEPT_DLL extern PIX * pixFilterComponentBySize(PIX * pixs, int32 rankorder, int32 type, int32 connectivity, BOX ** pbox);
LEPT_DLL extern PIX * pixMakeSymmetricMask(int32 w, int32 h, float hf, float vf, int32 type);
LEPT_DLL extern PIX * pixMakeFrameMask(int32 w, int32 h, float hf1, float hf2, float vf1, float vf2);
LEPT_DLL extern PIX * pixMakeCoveringOfRectangles(PIX * pixs, int32 maxiters);
LEPT_DLL extern l_ok pixFractionFgInMask(PIX * pix1, PIX * pix2, float * pfract);
LEPT_DLL extern l_ok pixClipToForeground(PIX * pixs, PIX ** ppixd, BOX ** pbox);
LEPT_DLL extern l_ok pixTestClipToForeground(PIX * pixs, int32 * pcanclip);
LEPT_DLL extern l_ok pixClipBoxToForeground(PIX * pixs, BOX * boxs, PIX ** ppixd, BOX ** pboxd);
LEPT_DLL extern l_ok pixScanForForeground(PIX * pixs, BOX * box, int32 scanflag, int32 * ploc);
LEPT_DLL extern l_ok pixClipBoxToEdges(PIX * pixs, BOX * boxs, int32 lowthresh, int32 highthresh, int32 maxwidth, int32 factor, PIX ** ppixd, BOX ** pboxd);
LEPT_DLL extern l_ok pixScanForEdge(PIX * pixs, BOX * box, int32 lowthresh, int32 highthresh, int32 maxwidth, int32 factor, int32 scanflag, int32 * ploc);
LEPT_DLL extern NUMA * pixExtractOnLine(PIX * pixs, int32 x1, int32 y1, int32 x2, int32 y2, int32 factor);
LEPT_DLL extern float pixAverageOnLine(PIX * pixs, int32 x1, int32 y1, int32 x2, int32 y2, int32 factor);
LEPT_DLL extern NUMA * pixAverageIntensityProfile(PIX * pixs, float fract, int32 dir, int32 first, int32 last, int32 factor1, int32 factor2);
LEPT_DLL extern NUMA * pixReversalProfile(PIX * pixs, float fract, int32 dir, int32 first, int32 last, int32 minreversal, int32 factor1, int32 factor2);
LEPT_DLL extern l_ok pixWindowedVarianceOnLine(PIX * pixs, int32 dir, int32 loc, int32 c1, int32 c2, int32 size, NUMA ** pnad);
LEPT_DLL extern l_ok pixMinMaxNearLine(PIX * pixs, int32 x1, int32 y1, int32 x2, int32 y2, int32 dist, int32 direction, NUMA ** pnamin, NUMA ** pnamax, float * pminave, float * pmaxave);
LEPT_DLL extern PIX * pixRankRowTransform(PIX * pixs);
LEPT_DLL extern PIX * pixRankColumnTransform(PIX * pixs);
LEPT_DLL extern PIXA * pixaCreate(int32 n);
LEPT_DLL extern PIXA * pixaCreateFromPix(PIX * pixs, int32 n, int32 cellw, int32 cellh);
LEPT_DLL extern PIXA * pixaCreateFromBoxa(PIX * pixs, BOXA * boxa, int32 start, int32 num, int32 * pcropwarn);
LEPT_DLL extern PIXA * pixaSplitPix(PIX * pixs, int32 nx, int32 ny, int32 borderwidth, uint32 bordercolor);
LEPT_DLL extern void pixaDestroy(PIXA ** ppixa);
LEPT_DLL extern PIXA * pixaCopy(PIXA * pixa, int32 copyflag);
LEPT_DLL extern l_ok pixaAddPix(PIXA * pixa, PIX * pix, int32 copyflag);
LEPT_DLL extern l_ok pixaAddBox(PIXA * pixa, BOX * box, int32 copyflag);
LEPT_DLL extern l_ok pixaExtendArrayToSize(PIXA * pixa, size_t size);
LEPT_DLL extern int32 pixaGetCount(PIXA * pixa);
LEPT_DLL extern l_ok pixaChangeRefcount(PIXA * pixa, int32 delta);
LEPT_DLL extern PIX * pixaGetPix(PIXA * pixa, int32 index, int32 accesstype);
LEPT_DLL extern l_ok pixaGetPixDimensions(PIXA * pixa, int32 index, int32 * pw, int32 * ph, int32 * pd);
LEPT_DLL extern BOXA * pixaGetBoxa(PIXA * pixa, int32 accesstype);
LEPT_DLL extern int32 pixaGetBoxaCount(PIXA * pixa);
LEPT_DLL extern BOX * pixaGetBox(PIXA * pixa, int32 index, int32 accesstype);
LEPT_DLL extern l_ok pixaGetBoxGeometry(PIXA * pixa, int32 index, int32 * px, int32 * py, int32 * pw, int32 * ph);
LEPT_DLL extern l_ok pixaSetBoxa(PIXA * pixa, BOXA * boxa, int32 accesstype);
LEPT_DLL extern PIX ** pixaGetPixArray(PIXA * pixa);
LEPT_DLL extern l_ok pixaVerifyDepth(PIXA * pixa, int32 * psame, int32 * pmaxd);
LEPT_DLL extern l_ok pixaVerifyDimensions(PIXA * pixa, int32 * psame, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern l_ok pixaIsFull(PIXA * pixa, int32 * pfullpa, int32 * pfullba);
LEPT_DLL extern l_ok pixaCountText(PIXA * pixa, int32 * pntext);
LEPT_DLL extern l_ok pixaSetText(PIXA * pixa, const char * text, SARRAY * sa);
LEPT_DLL extern void *** pixaGetLinePtrs(PIXA * pixa, int32 * psize);
LEPT_DLL extern l_ok pixaWriteStreamInfo(FILE * fp, PIXA * pixa);
LEPT_DLL extern l_ok pixaReplacePix(PIXA * pixa, int32 index, PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixaInsertPix(PIXA * pixa, int32 index, PIX * pixs, BOX * box);
LEPT_DLL extern l_ok pixaRemovePix(PIXA * pixa, int32 index);
LEPT_DLL extern l_ok pixaRemovePixAndSave(PIXA * pixa, int32 index, PIX ** ppix, BOX ** pbox);
LEPT_DLL extern l_ok pixaRemoveSelected(PIXA * pixa, NUMA * naindex);
LEPT_DLL extern l_ok pixaInitFull(PIXA * pixa, PIX * pix, BOX * box);
LEPT_DLL extern l_ok pixaClear(PIXA * pixa);
LEPT_DLL extern l_ok pixaJoin(PIXA * pixad, PIXA * pixas, int32 istart, int32 iend);
LEPT_DLL extern PIXA * pixaInterleave(PIXA * pixa1, PIXA * pixa2, int32 copyflag);
LEPT_DLL extern l_ok pixaaJoin(PIXAA * paad, PIXAA * paas, int32 istart, int32 iend);
LEPT_DLL extern PIXAA * pixaaCreate(int32 n);
LEPT_DLL extern PIXAA * pixaaCreateFromPixa(PIXA * pixa, int32 n, int32 type, int32 copyflag);
LEPT_DLL extern void pixaaDestroy(PIXAA ** ppaa);
LEPT_DLL extern l_ok pixaaAddPixa(PIXAA * paa, PIXA * pixa, int32 copyflag);
LEPT_DLL extern l_ok pixaaAddPix(PIXAA * paa, int32 index, PIX * pix, BOX * box, int32 copyflag);
LEPT_DLL extern l_ok pixaaAddBox(PIXAA * paa, BOX * box, int32 copyflag);
LEPT_DLL extern int32 pixaaGetCount(PIXAA * paa, NUMA ** pna);
LEPT_DLL extern PIXA * pixaaGetPixa(PIXAA * paa, int32 index, int32 accesstype);
LEPT_DLL extern BOXA * pixaaGetBoxa(PIXAA * paa, int32 accesstype);
LEPT_DLL extern PIX * pixaaGetPix(PIXAA * paa, int32 index, int32 ipix, int32 accessflag);
LEPT_DLL extern l_ok pixaaVerifyDepth(PIXAA * paa, int32 * psame, int32 * pmaxd);
LEPT_DLL extern l_ok pixaaVerifyDimensions(PIXAA * paa, int32 * psame, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern int32 pixaaIsFull(PIXAA * paa, int32 * pfull);
LEPT_DLL extern l_ok pixaaInitFull(PIXAA * paa, PIXA * pixa);
LEPT_DLL extern l_ok pixaaReplacePixa(PIXAA * paa, int32 index, PIXA * pixa);
LEPT_DLL extern l_ok pixaaClear(PIXAA * paa);
LEPT_DLL extern l_ok pixaaTruncate(PIXAA * paa);
LEPT_DLL extern PIXA * pixaRead(const char * filename);
LEPT_DLL extern PIXA * pixaReadStream(FILE * fp);
LEPT_DLL extern PIXA * pixaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixaWriteDebug(const char * fname, PIXA * pixa);
LEPT_DLL extern l_ok pixaWrite(const char * filename, PIXA * pixa);
LEPT_DLL extern l_ok pixaWriteStream(FILE * fp, PIXA * pixa);
LEPT_DLL extern l_ok pixaWriteMem(uint8 ** pdata, size_t * psize, PIXA * pixa);
LEPT_DLL extern PIXA * pixaReadBoth(const char * filename);
LEPT_DLL extern PIXAA * pixaaReadFromFiles(const char * dirname, const char * substr, int32 first, int32 nfiles);
LEPT_DLL extern PIXAA * pixaaRead(const char * filename);
LEPT_DLL extern PIXAA * pixaaReadStream(FILE * fp);
LEPT_DLL extern PIXAA * pixaaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixaaWrite(const char * filename, PIXAA * paa);
LEPT_DLL extern l_ok pixaaWriteStream(FILE * fp, PIXAA * paa);
LEPT_DLL extern l_ok pixaaWriteMem(uint8 ** pdata, size_t * psize, PIXAA * paa);
LEPT_DLL extern PIXACC * pixaccCreate(int32 w, int32 h, int32 negflag);
LEPT_DLL extern PIXACC * pixaccCreateFromPix(PIX * pix, int32 negflag);
LEPT_DLL extern void pixaccDestroy(PIXACC ** ppixacc);
LEPT_DLL extern PIX * pixaccFinal(PIXACC * pixacc, int32 outdepth);
LEPT_DLL extern PIX * pixaccGetPix(PIXACC * pixacc);
LEPT_DLL extern int32 pixaccGetOffset(PIXACC * pixacc);
LEPT_DLL extern l_ok pixaccAdd(PIXACC * pixacc, PIX * pix);
LEPT_DLL extern l_ok pixaccSubtract(PIXACC * pixacc, PIX * pix);
LEPT_DLL extern l_ok pixaccMultConst(PIXACC * pixacc, float factor);
LEPT_DLL extern l_ok pixaccMultConstAccumulate(PIXACC * pixacc, PIX * pix, float factor);
LEPT_DLL extern PIX * pixSelectBySize(PIX * pixs, int32 width, int32 height, int32 connectivity, int32 type, int32 relation, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectBySize(PIXA * pixas, int32 width, int32 height, int32 type, int32 relation, int32 * pchanged);
LEPT_DLL extern NUMA * pixaMakeSizeIndicator(PIXA * pixa, int32 width, int32 height, int32 type, int32 relation);
LEPT_DLL extern PIX * pixSelectByPerimToAreaRatio(PIX * pixs, float thresh, int32 connectivity, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByPerimToAreaRatio(PIXA * pixas, float thresh, int32 type, int32 * pchanged);
LEPT_DLL extern PIX * pixSelectByPerimSizeRatio(PIX * pixs, float thresh, int32 connectivity, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByPerimSizeRatio(PIXA * pixas, float thresh, int32 type, int32 * pchanged);
LEPT_DLL extern PIX * pixSelectByAreaFraction(PIX * pixs, float thresh, int32 connectivity, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByAreaFraction(PIXA * pixas, float thresh, int32 type, int32 * pchanged);
LEPT_DLL extern PIX * pixSelectByArea(PIX * pixs, float thresh, int32 connectivity, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByArea(PIXA * pixas, float thresh, int32 type, int32 * pchanged);
LEPT_DLL extern PIX * pixSelectByWidthHeightRatio(PIX * pixs, float thresh, int32 connectivity, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByWidthHeightRatio(PIXA * pixas, float thresh, int32 type, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectByNumConnComp(PIXA * pixas, int32 nmin, int32 nmax, int32 connectivity, int32 * pchanged);
LEPT_DLL extern PIXA * pixaSelectWithIndicator(PIXA * pixas, NUMA * na, int32 * pchanged);
LEPT_DLL extern l_ok pixRemoveWithIndicator(PIX * pixs, PIXA * pixa, NUMA * na);
LEPT_DLL extern l_ok pixAddWithIndicator(PIX * pixs, PIXA * pixa, NUMA * na);
LEPT_DLL extern PIXA * pixaSelectWithString(PIXA * pixas, const char * str, int32 * perror);
LEPT_DLL extern PIX * pixaRenderComponent(PIX * pixs, PIXA * pixa, int32 index);
LEPT_DLL extern PIXA * pixaSort(PIXA * pixas, int32 sorttype, int32 sortorder, NUMA ** pnaindex, int32 copyflag);
LEPT_DLL extern PIXA * pixaBinSort(PIXA * pixas, int32 sorttype, int32 sortorder, NUMA ** pnaindex, int32 copyflag);
LEPT_DLL extern PIXA * pixaSortByIndex(PIXA * pixas, NUMA * naindex, int32 copyflag);
LEPT_DLL extern PIXAA * pixaSort2dByIndex(PIXA * pixas, NUMAA * naa, int32 copyflag);
LEPT_DLL extern PIXA * pixaSelectRange(PIXA * pixas, int32 first, int32 last, int32 copyflag);
LEPT_DLL extern PIXAA * pixaaSelectRange(PIXAA * paas, int32 first, int32 last, int32 copyflag);
LEPT_DLL extern PIXAA * pixaaScaleToSize(PIXAA * paas, int32 wd, int32 hd);
LEPT_DLL extern PIXAA * pixaaScaleToSizeVar(PIXAA * paas, NUMA * nawd, NUMA * nahd);
LEPT_DLL extern PIXA * pixaScaleToSize(PIXA * pixas, int32 wd, int32 hd);
LEPT_DLL extern PIXA * pixaScaleToSizeRel(PIXA * pixas, int32 delw, int32 delh);
LEPT_DLL extern PIXA * pixaScale(PIXA * pixas, float scalex, float scaley);
LEPT_DLL extern PIXA * pixaScaleBySampling(PIXA * pixas, float scalex, float scaley);
LEPT_DLL extern PIXA * pixaRotate(PIXA * pixas, float angle, int32 type, int32 incolor, int32 width, int32 height);
LEPT_DLL extern PIXA * pixaRotateOrth(PIXA * pixas, int32 rotation);
LEPT_DLL extern PIXA * pixaTranslate(PIXA * pixas, int32 hshift, int32 vshift, int32 incolor);
LEPT_DLL extern PIXA * pixaAddBorderGeneral(PIXA * pixad, PIXA * pixas, int32 left, int32 right, int32 top, int32 bot,
    uint32 val);
LEPT_DLL extern PIXA * pixaaFlattenToPixa(PIXAA * paa, NUMA ** pnaindex, int32 copyflag);
LEPT_DLL extern l_ok pixaaSizeRange(PIXAA * paa, int32 * pminw, int32 * pminh, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern l_ok pixaSizeRange(PIXA * pixa, int32 * pminw, int32 * pminh, int32 * pmaxw, int32 * pmaxh);
LEPT_DLL extern PIXA * pixaClipToPix(PIXA * pixas, PIX * pixs);
LEPT_DLL extern l_ok pixaClipToForeground(PIXA * pixas, PIXA ** ppixad, BOXA ** pboxa);
LEPT_DLL extern l_ok pixaGetRenderingDepth(PIXA * pixa, int32 * pdepth);
LEPT_DLL extern l_ok pixaHasColor(PIXA * pixa, int32 * phascolor);
LEPT_DLL extern l_ok pixaAnyColormaps(PIXA * pixa, int32 * phascmap);
LEPT_DLL extern l_ok pixaGetDepthInfo(PIXA * pixa, int32 * pmaxdepth, int32 * psame);
LEPT_DLL extern PIXA * pixaConvertToSameDepth(PIXA * pixas);
LEPT_DLL extern PIXA * pixaConvertToGivenDepth(PIXA * pixas, int32 depth);
LEPT_DLL extern l_ok pixaEqual(PIXA * pixa1, PIXA * pixa2, int32 maxdist, NUMA ** pnaindex, int32 * psame);
LEPT_DLL extern l_ok pixaSetFullSizeBoxa(PIXA * pixa);
LEPT_DLL extern PIX * pixaDisplay(PIXA * pixa, int32 w, int32 h);
LEPT_DLL extern PIX * pixaDisplayRandomCmap(PIXA * pixa, int32 w, int32 h);
LEPT_DLL extern PIX * pixaDisplayLinearly(PIXA * pixas,
    int32 direction,
    float scalefactor,
    int32 background,
    int32 spacing,
    int32 border,
    BOXA ** pboxa);
LEPT_DLL extern PIX * pixaDisplayOnLattice(PIXA * pixa, int32 cellw, int32 cellh, int32 * pncols, BOXA ** pboxa);
LEPT_DLL extern PIX * pixaDisplayUnsplit(PIXA * pixa, int32 nx, int32 ny, int32 borderwidth, uint32 bordercolor);
LEPT_DLL extern PIX * pixaDisplayTiled(PIXA * pixa, int32 maxwidth, int32 background, int32 spacing);
LEPT_DLL extern PIX * pixaDisplayTiledInRows(PIXA * pixa,
    int32 outdepth,
    int32 maxwidth,
    float scalefactor,
    int32 background,
    int32 spacing,
    int32 border);
LEPT_DLL extern PIX * pixaDisplayTiledInColumns(PIXA * pixas, int32 nx, float scalefactor, int32 spacing, int32 border);
LEPT_DLL extern PIX * pixaDisplayTiledAndScaled(PIXA * pixa,
    int32 outdepth,
    int32 tilewidth,
    int32 ncols,
    int32 background,
    int32 spacing,
    int32 border);
LEPT_DLL extern PIX * pixaDisplayTiledWithText(PIXA * pixa,
    int32 maxwidth,
    float scalefactor,
    int32 spacing,
    int32 border,
    int32 fontsize,
    uint32 textcolor);
LEPT_DLL extern PIX * pixaDisplayTiledByIndex(PIXA * pixa,
    NUMA * na,
    int32 width,
    int32 spacing,
    int32 border,
    int32 fontsize,
    uint32 textcolor);
LEPT_DLL extern PIX * pixaDisplayPairTiledInColumns(PIXA * pixas1,
    PIXA * pixas2,
    int32 nx,
    float scalefactor,
    int32 spacing1,
    int32 spacing2,
    int32 border1,
    int32 border2,
    int32 fontsize,
    int32 startindex,
    SARRAY * sa);
LEPT_DLL extern PIX * pixaaDisplay(PIXAA * paa, int32 w, int32 h);
LEPT_DLL extern PIX * pixaaDisplayByPixa(PIXAA * paa,
    int32 maxnx,
    float scalefactor,
    int32 hspacing,
    int32 vspacing,
    int32 border);
LEPT_DLL extern PIXA * pixaaDisplayTiledAndScaled(PIXAA * paa,
    int32 outdepth,
    int32 tilewidth,
    int32 ncols,
    int32 background,
    int32 spacing,
    int32 border);
LEPT_DLL extern PIXA * pixaConvertTo1(PIXA * pixas, int32 thresh);
LEPT_DLL extern PIXA * pixaConvertTo8(PIXA * pixas, int32 cmapflag);
LEPT_DLL extern PIXA * pixaConvertTo8Colormap(PIXA * pixas, int32 dither);
LEPT_DLL extern PIXA * pixaConvertTo32(PIXA * pixas);
LEPT_DLL extern PIXA * pixaConstrainedSelect(PIXA * pixas, int32 first, int32 last, int32 nmax, int32 use_pairs, int32 copyflag);
LEPT_DLL extern l_ok pixaSelectToPdf(PIXA * pixas,
    int32 first,
    int32 last,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    uint32 color,
    int32 fontsize,
    const char * fileout);
LEPT_DLL extern PIXA * pixaMakeFromTiledPixa(PIXA * pixas, int32 w, int32 h, int32 nsamp);
LEPT_DLL extern PIXA * pixaMakeFromTiledPix(PIX * pixs, int32 w, int32 h, int32 start, int32 num, BOXA * boxa);
LEPT_DLL extern l_ok pixGetTileCount(PIX * pix, int32 * pn);
LEPT_DLL extern PIXA * pixaDisplayMultiTiled(PIXA * pixas,
    int32 nx,
    int32 ny,
    int32 maxw,
    int32 maxh,
    float scalefactor,
    int32 spacing,
    int32 border);
LEPT_DLL extern l_ok pixaSplitIntoFiles(PIXA * pixas,
    int32 nsplit,
    float scale,
    int32 outwidth,
    int32 write_pixa,
    int32 write_pix,
    int32 write_pdf);
LEPT_DLL extern l_ok convertToNUpFiles(const char * dir,
    const char * substr,
    int32 nx,
    int32 ny,
    int32 tw,
    int32 spacing,
    int32 border,
    int32 fontsize,
    const char * outdir);
LEPT_DLL extern PIXA * convertToNUpPixa(const char * dir,
    const char * substr,
    int32 nx,
    int32 ny,
    int32 tw,
    int32 spacing,
    int32 border,
    int32 fontsize);
LEPT_DLL extern PIXA * pixaConvertToNUpPixa(PIXA * pixas,
    SARRAY * sa,
    int32 nx,
    int32 ny,
    int32 tw,
    int32 spacing,
    int32 border,
    int32 fontsize);
LEPT_DLL extern l_ok pixaCompareInPdf(PIXA * pixa1,
    PIXA * pixa2,
    int32 nx,
    int32 ny,
    int32 tw,
    int32 spacing,
    int32 border,
    int32 fontsize,
    const char * fileout);
LEPT_DLL extern l_ok pmsCreate(size_t minsize, size_t smallest, NUMA * numalloc, const char * logfile);
LEPT_DLL extern void pmsDestroy(void);
LEPT_DLL extern void * pmsCustomAlloc(size_t nbytes);
LEPT_DLL extern void pmsCustomDealloc(void * data);
LEPT_DLL extern void * pmsGetAlloc(size_t nbytes);
LEPT_DLL extern l_ok pmsGetLevelForAlloc(size_t nbytes, int32 * plevel);
LEPT_DLL extern l_ok pmsGetLevelForDealloc(void * data, int32 * plevel);
LEPT_DLL extern void pmsLogInfo(void);
LEPT_DLL extern l_ok pixAddConstantGray(PIX * pixs, int32 val);
LEPT_DLL extern l_ok pixMultConstantGray(PIX * pixs, float val);
LEPT_DLL extern PIX * pixAddGray(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixSubtractGray(PIX * pixd, PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixMultiplyGray(PIX * pixs, PIX * pixg, float norm);
LEPT_DLL extern PIX * pixThresholdToValue(PIX * pixd, PIX * pixs, int32 threshval, int32 setval);
LEPT_DLL extern PIX * pixInitAccumulate(int32 w, int32 h, uint32 offset);
LEPT_DLL extern PIX * pixFinalAccumulate(PIX * pixs, uint32 offset, int32 depth);
LEPT_DLL extern PIX * pixFinalAccumulateThreshold(PIX * pixs, uint32 offset, uint32 threshold);
LEPT_DLL extern l_ok pixAccumulate(PIX * pixd, PIX * pixs, int32 op);
LEPT_DLL extern l_ok pixMultConstAccumulate(PIX * pixs, float factor, uint32 offset);
LEPT_DLL extern PIX * pixAbsDifference(PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixAddRGB(PIX * pixs1, PIX * pixs2);
LEPT_DLL extern PIX * pixMinOrMax(PIX * pixd, PIX * pixs1, PIX * pixs2, int32 type);
LEPT_DLL extern PIX * pixMaxDynamicRange(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixMaxDynamicRangeRGB(PIX * pixs, int32 type);
LEPT_DLL extern uint32 linearScaleRGBVal(uint32 sval, float factor);
LEPT_DLL extern uint32 logScaleRGBVal(uint32 sval, float * tab, float factor);
LEPT_DLL extern float * makeLogBase2Tab(void);
LEPT_DLL extern float getLogBase2(int32 val, float * logtab);
LEPT_DLL extern PIXC * pixcompCreateFromPix(PIX * pix, int32 comptype);
LEPT_DLL extern PIXC * pixcompCreateFromString(uint8 * data, size_t size, int32 copyflag);
LEPT_DLL extern PIXC * pixcompCreateFromFile(const char * filename, int32 comptype);
LEPT_DLL extern void pixcompDestroy(PIXC ** ppixc);
LEPT_DLL extern PIXC * pixcompCopy(PIXC * pixcs);
LEPT_DLL extern l_ok pixcompGetDimensions(PIXC * pixc, int32 * pw, int32 * ph, int32 * pd);
LEPT_DLL extern l_ok pixcompGetParameters(PIXC * pixc, int32 * pxres, int32 * pyres, int32 * pcomptype, int32 * pcmapflag);
LEPT_DLL extern l_ok pixcompDetermineFormat(int32 comptype, int32 d, int32 cmapflag, int32 * pformat);
LEPT_DLL extern PIX * pixCreateFromPixcomp(PIXC * pixc);
LEPT_DLL extern PIXAC * pixacompCreate(int32 n);
LEPT_DLL extern PIXAC * pixacompCreateWithInit(int32 n, int32 offset, PIX * pix, int32 comptype);
LEPT_DLL extern PIXAC * pixacompCreateFromPixa(PIXA * pixa, int32 comptype, int32 accesstype);
LEPT_DLL extern PIXAC * pixacompCreateFromFiles(const char * dirname, const char * substr, int32 comptype);
LEPT_DLL extern PIXAC * pixacompCreateFromSA(SARRAY * sa, int32 comptype);
LEPT_DLL extern void pixacompDestroy(PIXAC ** ppixac);
LEPT_DLL extern l_ok pixacompAddPix(PIXAC * pixac, PIX * pix, int32 comptype);
LEPT_DLL extern l_ok pixacompAddPixcomp(PIXAC * pixac, PIXC * pixc, int32 copyflag);
LEPT_DLL extern l_ok pixacompReplacePix(PIXAC * pixac, int32 index, PIX * pix, int32 comptype);
LEPT_DLL extern l_ok pixacompReplacePixcomp(PIXAC * pixac, int32 index, PIXC * pixc);
LEPT_DLL extern l_ok pixacompAddBox(PIXAC * pixac, BOX * box, int32 copyflag);
LEPT_DLL extern int32 pixacompGetCount(PIXAC * pixac);
LEPT_DLL extern PIXC * pixacompGetPixcomp(PIXAC * pixac, int32 index, int32 copyflag);
LEPT_DLL extern PIX * pixacompGetPix(PIXAC * pixac, int32 index);
LEPT_DLL extern l_ok pixacompGetPixDimensions(PIXAC * pixac, int32 index, int32 * pw, int32 * ph, int32 * pd);
LEPT_DLL extern BOXA * pixacompGetBoxa(PIXAC * pixac, int32 accesstype);
LEPT_DLL extern int32 pixacompGetBoxaCount(PIXAC * pixac);
LEPT_DLL extern BOX * pixacompGetBox(PIXAC * pixac, int32 index, int32 accesstype);
LEPT_DLL extern l_ok pixacompGetBoxGeometry(PIXAC * pixac, int32 index, int32 * px, int32 * py, int32 * pw, int32 * ph);
LEPT_DLL extern int32 pixacompGetOffset(PIXAC * pixac);
LEPT_DLL extern l_ok pixacompSetOffset(PIXAC * pixac, int32 offset);
LEPT_DLL extern PIXA * pixaCreateFromPixacomp(PIXAC * pixac, int32 accesstype);
LEPT_DLL extern l_ok pixacompJoin(PIXAC * pixacd, PIXAC * pixacs, int32 istart, int32 iend);
LEPT_DLL extern PIXAC * pixacompInterleave(PIXAC * pixac1, PIXAC * pixac2);
LEPT_DLL extern PIXAC * pixacompRead(const char * filename);
LEPT_DLL extern PIXAC * pixacompReadStream(FILE * fp);
LEPT_DLL extern PIXAC * pixacompReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixacompWrite(const char * filename, PIXAC * pixac);
LEPT_DLL extern l_ok pixacompWriteStream(FILE * fp, PIXAC * pixac);
LEPT_DLL extern l_ok pixacompWriteMem(uint8 ** pdata, size_t * psize, PIXAC * pixac);
LEPT_DLL extern l_ok pixacompConvertToPdf(PIXAC * pixac,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    const char * fileout);
LEPT_DLL extern l_ok pixacompConvertToPdfData(PIXAC * pixac,
    int32 res,
    float scalefactor,
    int32 type,
    int32 quality,
    const char * title,
    uint8 ** pdata,
    size_t * pnbytes);
LEPT_DLL extern l_ok pixacompFastConvertToPdfData(PIXAC * pixac, const char * title, uint8 ** pdata, size_t * pnbytes);
LEPT_DLL extern l_ok pixacompWriteStreamInfo(FILE * fp, PIXAC * pixac, const char * text);
LEPT_DLL extern l_ok pixcompWriteStreamInfo(FILE * fp, PIXC * pixc, const char * text);
LEPT_DLL extern PIX * pixacompDisplayTiledAndScaled(PIXAC * pixac,
    int32 outdepth,
    int32 tilewidth,
    int32 ncols,
    int32 background,
    int32 spacing,
    int32 border);
LEPT_DLL extern l_ok pixacompWriteFiles(PIXAC * pixac, const char * subdir);
LEPT_DLL extern l_ok pixcompWriteFile(const char * rootname, PIXC * pixc);
LEPT_DLL extern PIX * pixThreshold8(PIX * pixs, int32 d, int32 nlevels, int32 cmapflag);
LEPT_DLL extern PIX * pixRemoveColormapGeneral(PIX * pixs, int32 type, int32 ifnocmap);
LEPT_DLL extern PIX * pixRemoveColormap(PIX * pixs, int32 type);
LEPT_DLL extern l_ok pixAddGrayColormap8(PIX * pixs);
LEPT_DLL extern PIX * pixAddMinimalGrayColormap8(PIX * pixs);
LEPT_DLL extern PIX * pixConvertRGBToLuminance(PIX * pixs);
LEPT_DLL extern PIX * pixConvertRGBToGrayGeneral(PIX * pixs, int32 type, float rwt, float gwt, float bwt);
LEPT_DLL extern PIX * pixConvertRGBToGray(PIX * pixs, float rwt, float gwt, float bwt);
LEPT_DLL extern PIX * pixConvertRGBToGrayFast(PIX * pixs);
LEPT_DLL extern PIX * pixConvertRGBToGrayMinMax(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixConvertRGBToGraySatBoost(PIX * pixs, int32 refval);
LEPT_DLL extern PIX * pixConvertRGBToGrayArb(PIX * pixs, float rc, float gc, float bc);
LEPT_DLL extern PIX * pixConvertRGBToBinaryArb(PIX * pixs, float rc, float gc, float bc, int32 thresh, int32 relation);
LEPT_DLL extern PIX * pixConvertGrayToColormap(PIX * pixs);
LEPT_DLL extern PIX * pixConvertGrayToColormap8(PIX * pixs, int32 mindepth);
LEPT_DLL extern PIX * pixColorizeGray(PIX * pixs, uint32 color, int32 cmapflag);
LEPT_DLL extern PIX * pixConvertRGBToColormap(PIX * pixs, int32 ditherflag);
LEPT_DLL extern PIX * pixConvertCmapTo1(PIX * pixs);
LEPT_DLL extern l_ok pixQuantizeIfFewColors(PIX * pixs, int32 maxcolors, int32 mingraycolors, int32 octlevel, PIX ** ppixd);
LEPT_DLL extern PIX * pixConvert16To8(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixConvertGrayToFalseColor(PIX * pixs, float gamma);
LEPT_DLL extern PIX * pixUnpackBinary(PIX * pixs, int32 depth, int32 invert);
LEPT_DLL extern PIX * pixConvert1To16(PIX * pixd, PIX * pixs, uint16 val0, uint16 val1);
LEPT_DLL extern PIX * pixConvert1To32(PIX * pixd, PIX * pixs, uint32 val0, uint32 val1);
LEPT_DLL extern PIX * pixConvert1To2Cmap(PIX * pixs);
LEPT_DLL extern PIX * pixConvert1To2(PIX * pixd, PIX * pixs, int32 val0, int32 val1);
LEPT_DLL extern PIX * pixConvert1To4Cmap(PIX * pixs);
LEPT_DLL extern PIX * pixConvert1To4(PIX * pixd, PIX * pixs, int32 val0, int32 val1);
LEPT_DLL extern PIX * pixConvert1To8Cmap(PIX * pixs);
LEPT_DLL extern PIX * pixConvert1To8(PIX * pixd, PIX * pixs, uint8 val0, uint8 val1);
LEPT_DLL extern PIX * pixConvert2To8(PIX * pixs, uint8 val0, uint8 val1, uint8 val2, uint8 val3, int32 cmapflag);
LEPT_DLL extern PIX * pixConvert4To8(PIX * pixs, int32 cmapflag);
LEPT_DLL extern PIX * pixConvert8To16(PIX * pixs, int32 leftshift);
LEPT_DLL extern PIX * pixConvertTo2(PIX * pixs);
LEPT_DLL extern PIX * pixConvert8To2(PIX * pix);
LEPT_DLL extern PIX * pixConvertTo4(PIX * pixs);
LEPT_DLL extern PIX * pixConvert8To4(PIX * pix);
LEPT_DLL extern PIX * pixConvertTo1Adaptive(PIX * pixs);
LEPT_DLL extern PIX * pixConvertTo1(PIX * pixs, int32 threshold);
LEPT_DLL extern PIX * pixConvertTo1BySampling(PIX * pixs, int32 factor, int32 threshold);
LEPT_DLL extern PIX * pixConvertTo8(PIX * pixs, int32 cmapflag);
LEPT_DLL extern PIX * pixConvertTo8BySampling(PIX * pixs, int32 factor, int32 cmapflag);
LEPT_DLL extern PIX * pixConvertTo8Colormap(PIX * pixs, int32 dither);
LEPT_DLL extern PIX * pixConvertTo16(PIX * pixs);
LEPT_DLL extern PIX * pixConvertTo32(PIX * pixs);
LEPT_DLL extern PIX * pixConvertTo32BySampling(PIX * pixs, int32 factor);
LEPT_DLL extern PIX * pixConvert8To32(PIX * pixs);
LEPT_DLL extern PIX * pixConvertTo8Or32(PIX * pixs, int32 copyflag, int32 warnflag);
LEPT_DLL extern PIX * pixConvert24To32(PIX * pixs);
LEPT_DLL extern PIX * pixConvert32To24(PIX * pixs);
LEPT_DLL extern PIX * pixConvert32To16(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixConvert32To8(PIX * pixs, int32 type16, int32 type8);
LEPT_DLL extern PIX * pixRemoveAlpha(PIX * pixs);
LEPT_DLL extern PIX * pixAddAlphaTo1bpp(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixConvertLossless(PIX * pixs, int32 d);
LEPT_DLL extern PIX * pixConvertForPSWrap(PIX * pixs);
LEPT_DLL extern PIX * pixConvertToSubpixelRGB(PIX * pixs, float scalex, float scaley, int32 order);
LEPT_DLL extern PIX * pixConvertGrayToSubpixelRGB(PIX * pixs, float scalex, float scaley, int32 order);
LEPT_DLL extern PIX * pixConvertColorToSubpixelRGB(PIX * pixs, float scalex, float scaley, int32 order);
LEPT_DLL extern void l_setNeutralBoostVal(int32 val);
LEPT_DLL extern PIX * pixConnCompTransform(PIX * pixs, int32 connect, int32 depth);
LEPT_DLL extern PIX * pixConnCompAreaTransform(PIX * pixs, int32 connect);
LEPT_DLL extern l_ok pixConnCompIncrInit(PIX * pixs, int32 conn, PIX ** ppixd, PTAA ** pptaa, int32 * pncc);
LEPT_DLL extern int32 pixConnCompIncrAdd(PIX * pixs, PTAA * ptaa, int32 * pncc, float x, float y, int32 debug);
LEPT_DLL extern l_ok pixGetSortedNeighborValues(PIX * pixs, int32 x, int32 y, int32 conn, int32 ** pneigh, int32 * pnvals);
LEPT_DLL extern PIX * pixLocToColorTransform(PIX * pixs);
LEPT_DLL extern PIXTILING * pixTilingCreate(PIX * pixs, int32 nx, int32 ny, int32 w, int32 h, int32 xoverlap, int32 yoverlap);
LEPT_DLL extern void pixTilingDestroy(PIXTILING ** ppt);
LEPT_DLL extern l_ok pixTilingGetCount(PIXTILING * pt, int32 * pnx, int32 * pny);
LEPT_DLL extern l_ok pixTilingGetSize(PIXTILING * pt, int32 * pw, int32 * ph);
LEPT_DLL extern PIX * pixTilingGetTile(PIXTILING * pt, int32 i, int32 j);
LEPT_DLL extern l_ok pixTilingNoStripOnPaint(PIXTILING * pt);
LEPT_DLL extern l_ok pixTilingPaintTile(PIX * pixd, int32 i, int32 j, PIX * pixs, PIXTILING * pt);
LEPT_DLL extern PIX * pixReadStreamPng(FILE * fp);
LEPT_DLL extern l_ok readHeaderPng(const char * filename, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * piscmap);
LEPT_DLL extern l_ok freadHeaderPng(FILE * fp, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * piscmap);
LEPT_DLL extern l_ok readHeaderMemPng(const uint8 * data,
    size_t size,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * piscmap);
LEPT_DLL extern int32 fgetPngResolution(FILE * fp, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok isPngInterlaced(const char * filename, int32 * pinterlaced);
LEPT_DLL extern l_ok fgetPngColormapInfo(FILE * fp, PIXCMAP ** pcmap, int32 * ptransparency);
LEPT_DLL extern l_ok pixWritePng(const char * filename, PIX * pix, float gamma);
LEPT_DLL extern l_ok pixWriteStreamPng(FILE * fp, PIX * pix, float gamma);
LEPT_DLL extern l_ok pixSetZlibCompression(PIX * pix, int32 compval);
LEPT_DLL extern void l_pngSetReadStrip16To8(int32 flag);
LEPT_DLL extern PIX * pixReadMemPng(const uint8 * filedata, size_t filesize);
LEPT_DLL extern l_ok pixWriteMemPng(uint8 ** pfiledata, size_t * pfilesize, PIX * pix, float gamma);
LEPT_DLL extern PIX * pixReadStreamPnm(FILE * fp);
LEPT_DLL extern l_ok readHeaderPnm(const char * filename,
    int32 * pw,
    int32 * ph,
    int32 * pd,
    int32 * ptype,
    int32 * pbps,
    int32 * pspp);
LEPT_DLL extern l_ok freadHeaderPnm(FILE * fp, int32 * pw, int32 * ph, int32 * pd, int32 * ptype, int32 * pbps, int32 * pspp);
LEPT_DLL extern l_ok pixWriteStreamPnm(FILE * fp, PIX * pix);
LEPT_DLL extern l_ok pixWriteStreamAsciiPnm(FILE * fp, PIX * pix);
LEPT_DLL extern l_ok pixWriteStreamPam(FILE * fp, PIX * pix);
LEPT_DLL extern PIX * pixReadMemPnm(const uint8 * data, size_t size);
LEPT_DLL extern l_ok readHeaderMemPnm(const uint8 * data,
    size_t size,
    int32 * pw,
    int32 * ph,
    int32 * pd,
    int32 * ptype,
    int32 * pbps,
    int32 * pspp);
LEPT_DLL extern l_ok pixWriteMemPnm(uint8 ** pdata, size_t * psize, PIX * pix);
LEPT_DLL extern l_ok pixWriteMemPam(uint8 ** pdata, size_t * psize, PIX * pix);
LEPT_DLL extern PIX * pixProjectiveSampledPta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixProjectiveSampled(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixProjectivePta(PIX * pixs, PTA * ptad, PTA * ptas, int32 incolor);
LEPT_DLL extern PIX * pixProjective(PIX * pixs, float * vc, int32 incolor);
LEPT_DLL extern PIX * pixProjectivePtaColor(PIX * pixs, PTA * ptad, PTA * ptas, uint32 colorval);
LEPT_DLL extern PIX * pixProjectiveColor(PIX * pixs, float * vc, uint32 colorval);
LEPT_DLL extern PIX * pixProjectivePtaGray(PIX * pixs, PTA * ptad, PTA * ptas, uint8 grayval);
LEPT_DLL extern PIX * pixProjectiveGray(PIX * pixs, float * vc, uint8 grayval);
LEPT_DLL extern PIX * pixProjectivePtaWithAlpha(PIX * pixs, PTA * ptad, PTA * ptas, PIX * pixg, float fract, int32 border);
LEPT_DLL extern l_ok getProjectiveXformCoeffs(PTA * ptas, PTA * ptad, float ** pvc);
LEPT_DLL extern l_ok projectiveXformSampledPt(float * vc, int32 x, int32 y, int32 * pxp, int32 * pyp);
LEPT_DLL extern l_ok projectiveXformPt(float * vc, int32 x, int32 y, float * pxp, float * pyp);
LEPT_DLL extern l_ok convertFilesToPS(const char * dirin, const char * substr, int32 res, const char * fileout);
LEPT_DLL extern l_ok sarrayConvertFilesToPS(SARRAY * sa, int32 res, const char * fileout);
LEPT_DLL extern l_ok convertFilesFittedToPS(const char * dirin, const char * substr, float xpts, float ypts, const char * fileout);
LEPT_DLL extern l_ok sarrayConvertFilesFittedToPS(SARRAY * sa, float xpts, float ypts, const char * fileout);
LEPT_DLL extern l_ok writeImageCompressedToPSFile(const char * filein, const char * fileout, int32 res, int32 * pindex);
LEPT_DLL extern l_ok convertSegmentedPagesToPS(const char * pagedir,
    const char * pagestr,
    int32 page_numpre,
    const char * maskdir,
    const char * maskstr,
    int32 mask_numpre,
    int32 numpost,
    int32 maxnum,
    float textscale,
    float imagescale,
    int32 threshold,
    const char * fileout);
LEPT_DLL extern l_ok pixWriteSegmentedPageToPS(PIX * pixs,
    PIX * pixm,
    float textscale,
    float imagescale,
    int32 threshold,
    int32 pageno,
    const char * fileout);
LEPT_DLL extern l_ok pixWriteMixedToPS(PIX * pixb, PIX * pixc, float scale, int32 pageno, const char * fileout);
LEPT_DLL extern l_ok convertToPSEmbed(const char * filein, const char * fileout, int32 level);
LEPT_DLL extern l_ok pixaWriteCompressedToPS(PIXA * pixa, const char * fileout, int32 res, int32 level);
LEPT_DLL extern l_ok pixWriteCompressedToPS(PIX * pix, const char * fileout, int32 res, int32 level, int32 * pindex);
LEPT_DLL extern l_ok pixWritePSEmbed(const char * filein, const char * fileout);
LEPT_DLL extern l_ok pixWriteStreamPS(FILE * fp, PIX * pix, BOX * box, int32 res, float scale);
LEPT_DLL extern char * pixWriteStringPS(PIX * pixs, BOX * box, int32 res, float scale);
LEPT_DLL extern char * generateUncompressedPS(char * hexdata,
    int32 w,
    int32 h,
    int32 d,
    int32 psbpl,
    int32 bps,
    float xpt,
    float ypt,
    float wpt,
    float hpt,
    int32 boxflag);
LEPT_DLL extern l_ok convertJpegToPSEmbed(const char * filein, const char * fileout);
LEPT_DLL extern l_ok convertJpegToPS(const char * filein,
    const char * fileout,
    const char * operation,
    int32 x,
    int32 y,
    int32 res,
    float scale,
    int32 pageno,
    int32 endpage);
LEPT_DLL extern l_ok convertG4ToPSEmbed(const char * filein, const char * fileout);
LEPT_DLL extern l_ok convertG4ToPS(const char * filein,
    const char * fileout,
    const char * operation,
    int32 x,
    int32 y,
    int32 res,
    float scale,
    int32 pageno,
    int32 maskflag,
    int32 endpage);
LEPT_DLL extern l_ok convertTiffMultipageToPS(const char * filein, const char * fileout, float fillfract);
LEPT_DLL extern l_ok convertFlateToPSEmbed(const char * filein, const char * fileout);
LEPT_DLL extern l_ok convertFlateToPS(const char * filein,
    const char * fileout,
    const char * operation,
    int32 x,
    int32 y,
    int32 res,
    float scale,
    int32 pageno,
    int32 endpage);
LEPT_DLL extern l_ok pixWriteMemPS(uint8 ** pdata, size_t * psize, PIX * pix, BOX * box, int32 res, float scale);
LEPT_DLL extern int32 getResLetterPage(int32 w, int32 h, float fillfract);
LEPT_DLL extern int32 getResA4Page(int32 w, int32 h, float fillfract);
LEPT_DLL extern void l_psWriteBoundingBox(int32 flag);
LEPT_DLL extern PTA * ptaCreate(int32 n);
LEPT_DLL extern PTA * ptaCreateFromNuma(NUMA * nax, NUMA * nay);
LEPT_DLL extern void ptaDestroy(PTA ** ppta);
LEPT_DLL extern PTA * ptaCopy(PTA * pta);
LEPT_DLL extern PTA * ptaCopyRange(PTA * ptas, int32 istart, int32 iend);
LEPT_DLL extern PTA * ptaClone(PTA * pta);
LEPT_DLL extern l_ok ptaEmpty(PTA * pta);
LEPT_DLL extern l_ok ptaAddPt(PTA * pta, float x, float y);
LEPT_DLL extern l_ok ptaInsertPt(PTA * pta, int32 index, int32 x, int32 y);
LEPT_DLL extern l_ok ptaRemovePt(PTA * pta, int32 index);
LEPT_DLL extern int32 ptaGetRefcount(PTA * pta);
LEPT_DLL extern int32 ptaChangeRefcount(PTA * pta, int32 delta);
LEPT_DLL extern int32 ptaGetCount(PTA * pta);
LEPT_DLL extern l_ok ptaGetPt(PTA * pta, int32 index, float * px, float * py);
LEPT_DLL extern l_ok ptaGetIPt(PTA * pta, int32 index, int32 * px, int32 * py);
LEPT_DLL extern l_ok ptaSetPt(PTA * pta, int32 index, float x, float y);
LEPT_DLL extern l_ok ptaGetArrays(PTA * pta, NUMA ** pnax, NUMA ** pnay);
LEPT_DLL extern PTA * ptaRead(const char * filename);
LEPT_DLL extern PTA * ptaReadStream(FILE * fp);
LEPT_DLL extern PTA * ptaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok ptaWriteDebug(const char * filename, PTA * pta, int32 type);
LEPT_DLL extern l_ok ptaWrite(const char * filename, PTA * pta, int32 type);
LEPT_DLL extern l_ok ptaWriteStream(FILE * fp, PTA * pta, int32 type);
LEPT_DLL extern l_ok ptaWriteMem(uint8 ** pdata, size_t * psize, PTA * pta, int32 type);
LEPT_DLL extern PTAA * ptaaCreate(int32 n);
LEPT_DLL extern void ptaaDestroy(PTAA ** pptaa);
LEPT_DLL extern l_ok ptaaAddPta(PTAA * ptaa, PTA * pta, int32 copyflag);
LEPT_DLL extern int32 ptaaGetCount(PTAA * ptaa);
LEPT_DLL extern PTA * ptaaGetPta(PTAA * ptaa, int32 index, int32 accessflag);
LEPT_DLL extern l_ok ptaaGetPt(PTAA * ptaa, int32 ipta, int32 jpt, float * px, float * py);
LEPT_DLL extern l_ok ptaaInitFull(PTAA * ptaa, PTA * pta);
LEPT_DLL extern l_ok ptaaReplacePta(PTAA * ptaa, int32 index, PTA * pta);
LEPT_DLL extern l_ok ptaaAddPt(PTAA * ptaa, int32 ipta, float x, float y);
LEPT_DLL extern l_ok ptaaTruncate(PTAA * ptaa);
LEPT_DLL extern PTAA * ptaaRead(const char * filename);
LEPT_DLL extern PTAA * ptaaReadStream(FILE * fp);
LEPT_DLL extern PTAA * ptaaReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok ptaaWriteDebug(const char * filename, PTAA * ptaa, int32 type);
LEPT_DLL extern l_ok ptaaWrite(const char * filename, PTAA * ptaa, int32 type);
LEPT_DLL extern l_ok ptaaWriteStream(FILE * fp, PTAA * ptaa, int32 type);
LEPT_DLL extern l_ok ptaaWriteMem(uint8 ** pdata, size_t * psize, PTAA * ptaa, int32 type);
LEPT_DLL extern PTA * ptaSubsample(PTA * ptas, int32 subfactor);
LEPT_DLL extern l_ok ptaJoin(PTA * ptad, PTA * ptas, int32 istart, int32 iend);
LEPT_DLL extern l_ok ptaaJoin(PTAA * ptaad, PTAA * ptaas, int32 istart, int32 iend);
LEPT_DLL extern PTA * ptaReverse(PTA * ptas, int32 type);
LEPT_DLL extern PTA * ptaTranspose(PTA * ptas);
LEPT_DLL extern PTA * ptaCyclicPerm(PTA * ptas, int32 xs, int32 ys);
LEPT_DLL extern PTA * ptaSelectRange(PTA * ptas, int32 first, int32 last);
LEPT_DLL extern BOX * ptaGetBoundingRegion(PTA * pta);
LEPT_DLL extern l_ok ptaGetRange(PTA * pta, float * pminx, float * pmaxx, float * pminy, float * pmaxy);
LEPT_DLL extern PTA * ptaGetInsideBox(PTA * ptas, BOX * box);
LEPT_DLL extern PTA * pixFindCornerPixels(PIX * pixs);
LEPT_DLL extern int32 ptaContainsPt(PTA * pta, int32 x, int32 y);
LEPT_DLL extern int32 ptaTestIntersection(PTA * pta1, PTA * pta2);
LEPT_DLL extern PTA * ptaTransform(PTA * ptas, int32 shiftx, int32 shifty, float scalex, float scaley);
LEPT_DLL extern int32 ptaPtInsidePolygon(PTA * pta, float x, float y, int32 * pinside);
LEPT_DLL extern float l_angleBetweenVectors(float x1, float y1, float x2, float y2);
LEPT_DLL extern int32 ptaPolygonIsConvex(PTA * pta, int32 * pisconvex);
LEPT_DLL extern l_ok ptaGetMinMax(PTA * pta, float * pxmin, float * pymin, float * pxmax, float * pymax);
LEPT_DLL extern PTA * ptaSelectByValue(PTA * ptas, float xth, float yth, int32 type, int32 relation);
LEPT_DLL extern PTA * ptaCropToMask(PTA * ptas, PIX * pixm);
LEPT_DLL extern l_ok ptaGetLinearLSF(PTA * pta, float * pa, float * pb, NUMA ** pnafit);
LEPT_DLL extern l_ok ptaGetQuadraticLSF(PTA * pta, float * pa, float * pb, float * pc, NUMA ** pnafit);
LEPT_DLL extern l_ok ptaGetCubicLSF(PTA * pta, float * pa, float * pb, float * pc, float * pd, NUMA ** pnafit);
LEPT_DLL extern l_ok ptaGetQuarticLSF(PTA * pta,
    float * pa,
    float * pb,
    float * pc,
    float * pd,
    float * pe,
    NUMA ** pnafit);
LEPT_DLL extern l_ok ptaNoisyLinearLSF(PTA * pta,
    float factor,
    PTA ** pptad,
    float * pa,
    float * pb,
    float * pmederr,
    NUMA ** pnafit);
LEPT_DLL extern l_ok ptaNoisyQuadraticLSF(PTA * pta,
    float factor,
    PTA ** pptad,
    float * pa,
    float * pb,
    float * pc,
    float * pmederr,
    NUMA ** pnafit);
LEPT_DLL extern l_ok applyLinearFit(float a, float b, float x, float * py);
LEPT_DLL extern l_ok applyQuadraticFit(float a, float b, float c, float x, float * py);
LEPT_DLL extern l_ok applyCubicFit(float a, float b, float c, float d, float x, float * py);
LEPT_DLL extern l_ok applyQuarticFit(float a, float b, float c, float d, float e, float x, float * py);
LEPT_DLL extern l_ok pixPlotAlongPta(PIX * pixs, PTA * pta, int32 outformat, const char * title);
LEPT_DLL extern PTA * ptaGetPixelsFromPix(PIX * pixs, BOX * box);
LEPT_DLL extern PIX * pixGenerateFromPta(PTA * pta, int32 w, int32 h);
LEPT_DLL extern PTA * ptaGetBoundaryPixels(PIX * pixs, int32 type);
LEPT_DLL extern PTAA * ptaaGetBoundaryPixels(PIX * pixs, int32 type, int32 connectivity, BOXA ** pboxa, PIXA ** ppixa);
LEPT_DLL extern PTAA * ptaaIndexLabeledPixels(PIX * pixs, int32 * pncc);
LEPT_DLL extern PTA * ptaGetNeighborPixLocs(PIX * pixs, int32 x, int32 y, int32 conn);
LEPT_DLL extern PTA * numaConvertToPta1(NUMA * na);
LEPT_DLL extern PTA * numaConvertToPta2(NUMA * nax, NUMA * nay);
LEPT_DLL extern l_ok ptaConvertToNuma(PTA * pta, NUMA ** pnax, NUMA ** pnay);
LEPT_DLL extern PIX * pixDisplayPta(PIX * pixd, PIX * pixs, PTA * pta);
LEPT_DLL extern PIX * pixDisplayPtaaPattern(PIX * pixd, PIX * pixs, PTAA * ptaa, PIX * pixp, int32 cx, int32 cy);
LEPT_DLL extern PIX * pixDisplayPtaPattern(PIX * pixd, PIX * pixs, PTA * pta, PIX * pixp, int32 cx, int32 cy, uint32 color);
LEPT_DLL extern PTA * ptaReplicatePattern(PTA * ptas, PIX * pixp, PTA * ptap, int32 cx, int32 cy, int32 w, int32 h);
LEPT_DLL extern PIX * pixDisplayPtaa(PIX * pixs, PTAA * ptaa);
LEPT_DLL extern PTA * ptaSort(PTA * ptas, int32 sorttype, int32 sortorder, NUMA ** pnaindex);
LEPT_DLL extern l_ok ptaGetSortIndex(PTA * ptas, int32 sorttype, int32 sortorder, NUMA ** pnaindex);
LEPT_DLL extern PTA * ptaSortByIndex(PTA * ptas, NUMA * naindex);
LEPT_DLL extern PTAA * ptaaSortByIndex(PTAA * ptaas, NUMA * naindex);
LEPT_DLL extern l_ok ptaGetRankValue(PTA * pta, float fract, PTA * ptasort, int32 sorttype, float * pval);
LEPT_DLL extern PTA * ptaSort2d(PTA * pta);
LEPT_DLL extern l_ok ptaEqual(PTA * pta1, PTA * pta2, int32 * psame);
LEPT_DLL extern L_ASET * l_asetCreateFromPta(PTA * pta);
LEPT_DLL extern l_ok ptaRemoveDupsByAset(PTA * ptas, PTA ** pptad);
LEPT_DLL extern l_ok ptaUnionByAset(PTA * pta1, PTA * pta2, PTA ** pptad);
LEPT_DLL extern l_ok ptaIntersectionByAset(PTA * pta1, PTA * pta2, PTA ** pptad);
LEPT_DLL extern L_HASHMAP * l_hmapCreateFromPta(PTA * pta);
LEPT_DLL extern l_ok ptaRemoveDupsByHmap(PTA * ptas, PTA ** pptad, L_HASHMAP ** phmap);
LEPT_DLL extern l_ok ptaUnionByHmap(PTA * pta1, PTA * pta2, PTA ** pptad);
LEPT_DLL extern l_ok ptaIntersectionByHmap(PTA * pta1, PTA * pta2, PTA ** pptad);
LEPT_DLL extern L_PTRA * ptraCreate(int32 n);
LEPT_DLL extern void ptraDestroy(L_PTRA ** ppa, int32 freeflag, int32 warnflag);
LEPT_DLL extern l_ok ptraAdd(L_PTRA * pa, void * item);
LEPT_DLL extern l_ok ptraInsert(L_PTRA * pa, int32 index, void * item, int32 shiftflag);
LEPT_DLL extern void * ptraRemove(L_PTRA * pa, int32 index, int32 flag);
LEPT_DLL extern void * ptraRemoveLast(L_PTRA * pa);
LEPT_DLL extern void * ptraReplace(L_PTRA * pa, int32 index, void * item, int32 freeflag);
LEPT_DLL extern l_ok ptraSwap(L_PTRA * pa, int32 index1, int32 index2);
LEPT_DLL extern l_ok ptraCompactArray(L_PTRA * pa);
LEPT_DLL extern l_ok ptraReverse(L_PTRA * pa);
LEPT_DLL extern l_ok ptraJoin(L_PTRA * pa1, L_PTRA * pa2);
LEPT_DLL extern l_ok ptraGetMaxIndex(L_PTRA * pa, int32 * pmaxindex);
LEPT_DLL extern l_ok ptraGetActualCount(L_PTRA * pa, int32 * pcount);
LEPT_DLL extern void * ptraGetPtrToItem(L_PTRA * pa, int32 index);
LEPT_DLL extern L_PTRAA * ptraaCreate(int32 n);
LEPT_DLL extern void ptraaDestroy(L_PTRAA ** ppaa, int32 freeflag, int32 warnflag);
LEPT_DLL extern l_ok ptraaGetSize(L_PTRAA * paa, int32 * psize);
LEPT_DLL extern l_ok ptraaInsertPtra(L_PTRAA * paa, int32 index, L_PTRA * pa);
LEPT_DLL extern L_PTRA * ptraaGetPtra(L_PTRAA * paa, int32 index, int32 accessflag);
LEPT_DLL extern L_PTRA * ptraaFlattenToPtra(L_PTRAA * paa);
LEPT_DLL extern l_ok pixQuadtreeMean(PIX * pixs, int32 nlevels, PIX * pix_ma, FPIXA ** pfpixa);
LEPT_DLL extern l_ok pixQuadtreeVariance(PIX * pixs, int32 nlevels, PIX * pix_ma, DPIX * dpix_msa, FPIXA ** pfpixa_v, FPIXA ** pfpixa_rv);
LEPT_DLL extern l_ok pixMeanInRectangle(PIX * pixs, BOX * box, PIX * pixma, float * pval);
LEPT_DLL extern l_ok pixVarianceInRectangle(PIX * pixs, BOX * box, PIX * pix_ma, DPIX * dpix_msa, float * pvar, float * prvar);
LEPT_DLL extern BOXAA * boxaaQuadtreeRegions(int32 w, int32 h, int32 nlevels);
LEPT_DLL extern l_ok quadtreeGetParent(FPIXA * fpixa, int32 level, int32 x, int32 y, float * pval);
LEPT_DLL extern l_ok quadtreeGetChildren(FPIXA * fpixa,
    int32 level,
    int32 x,
    int32 y,
    float * pval00,
    float * pval10,
    float * pval01,
    float * pval11);
LEPT_DLL extern int32 quadtreeMaxLevels(int32 w, int32 h);
LEPT_DLL extern PIX * fpixaDisplayQuadtree(FPIXA * fpixa, int32 factor, int32 fontsize);
LEPT_DLL extern L_QUEUE * lqueueCreate(int32 nalloc);
LEPT_DLL extern void lqueueDestroy(L_QUEUE ** plq, int32 freeflag);
LEPT_DLL extern l_ok lqueueAdd(L_QUEUE * lq, void * item);
LEPT_DLL extern void * lqueueRemove(L_QUEUE * lq);
LEPT_DLL extern int32 lqueueGetCount(L_QUEUE * lq);
LEPT_DLL extern l_ok lqueuePrint(FILE * fp, L_QUEUE * lq);
LEPT_DLL extern PIX * pixRankFilter(PIX * pixs, int32 wf, int32 hf, float rank);
LEPT_DLL extern PIX * pixRankFilterRGB(PIX * pixs, int32 wf, int32 hf, float rank);
LEPT_DLL extern PIX * pixRankFilterGray(PIX * pixs, int32 wf, int32 hf, float rank);
LEPT_DLL extern PIX * pixMedianFilter(PIX * pixs, int32 wf, int32 hf);
LEPT_DLL extern PIX * pixRankFilterWithScaling(PIX * pixs, int32 wf, int32 hf, float rank, float scalefactor);
LEPT_DLL extern L_RBTREE * l_rbtreeCreate(int32 keytype);
LEPT_DLL extern RB_TYPE * l_rbtreeLookup(L_RBTREE * t, RB_TYPE key);
LEPT_DLL extern void l_rbtreeInsert(L_RBTREE * t, RB_TYPE key, RB_TYPE value);
LEPT_DLL extern void l_rbtreeDelete(L_RBTREE * t, RB_TYPE key);
LEPT_DLL extern void l_rbtreeDestroy(L_RBTREE ** pt);
LEPT_DLL extern L_RBTREE_NODE * l_rbtreeGetFirst(L_RBTREE * t);
LEPT_DLL extern L_RBTREE_NODE * l_rbtreeGetNext(L_RBTREE_NODE * n);
LEPT_DLL extern L_RBTREE_NODE * l_rbtreeGetLast(L_RBTREE * t);
LEPT_DLL extern L_RBTREE_NODE * l_rbtreeGetPrev(L_RBTREE_NODE * n);
LEPT_DLL extern int32 l_rbtreeGetCount(L_RBTREE * t);
LEPT_DLL extern void l_rbtreePrint(FILE * fp, L_RBTREE * t);
LEPT_DLL extern SARRAY * pixProcessBarcodes(PIX * pixs, int32 format, int32 method, SARRAY ** psaw, int32 debugflag);
LEPT_DLL extern PIXA * pixExtractBarcodes(PIX * pixs, int32 debugflag);
LEPT_DLL extern SARRAY * pixReadBarcodes(PIXA * pixa, int32 format, int32 method, SARRAY ** psaw, int32 debugflag);
LEPT_DLL extern NUMA * pixReadBarcodeWidths(PIX * pixs, int32 method, int32 debugflag);
LEPT_DLL extern BOXA * pixLocateBarcodes(PIX * pixs, int32 thresh, PIX ** ppixb, PIX ** ppixm);
LEPT_DLL extern PIX * pixDeskewBarcode(PIX * pixs,
    PIX * pixb,
    BOX * box,
    int32 margin,
    int32 threshold,
    float * pangle,
    float * pconf);
LEPT_DLL extern NUMA * pixExtractBarcodeWidths1(PIX * pixs,
    float thresh,
    float binfract,
    NUMA ** pnaehist,
    NUMA ** pnaohist,
    int32 debugflag);
LEPT_DLL extern NUMA * pixExtractBarcodeWidths2(PIX * pixs, float thresh, float * pwidth, NUMA ** pnac, int32 debugflag);
LEPT_DLL extern NUMA * pixExtractBarcodeCrossings(PIX * pixs, float thresh, int32 debugflag);
LEPT_DLL extern NUMA * numaQuantizeCrossingsByWidth(NUMA * nas, float binfract, NUMA ** pnaehist, NUMA ** pnaohist, int32 debugflag);
LEPT_DLL extern NUMA * numaQuantizeCrossingsByWindow(NUMA * nas,
    float ratio,
    float * pwidth,
    float * pfirstloc,
    NUMA ** pnac,
    int32 debugflag);
LEPT_DLL extern PIXA * pixaReadFiles(const char * dirname, const char * substr);
LEPT_DLL extern PIXA * pixaReadFilesSA(SARRAY * sa);
LEPT_DLL extern PIX * pixRead(const char * filename);
LEPT_DLL extern PIX * pixReadWithHint(const char * filename, int32 hint);
LEPT_DLL extern PIX * pixReadIndexed(SARRAY * sa, int32 index);
LEPT_DLL extern PIX * pixReadStream(FILE * fp, int32 hint);
LEPT_DLL extern l_ok pixReadHeader(const char * filename,
    int32 * pformat,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * piscmap);
LEPT_DLL extern l_ok findFileFormat(const char * filename, int32 * pformat);
LEPT_DLL extern l_ok findFileFormatStream(FILE * fp, int32 * pformat);
LEPT_DLL extern l_ok findFileFormatBuffer(const uint8 * buf, int32 * pformat);
LEPT_DLL extern int32 fileFormatIsTiff(FILE * fp);
LEPT_DLL extern PIX * pixReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixReadHeaderMem(const uint8 * data,
    size_t size,
    int32 * pformat,
    int32 * pw,
    int32 * ph,
    int32 * pbps,
    int32 * pspp,
    int32 * piscmap);
LEPT_DLL extern l_ok writeImageFileInfo(const char * filename, FILE * fpout, int32 headeronly);
LEPT_DLL extern l_ok ioFormatTest(const char * filename);
LEPT_DLL extern L_RECOG * recogCreateFromRecog(L_RECOG * recs,
    int32 scalew,
    int32 scaleh,
    int32 linew,
    int32 threshold,
    int32 maxyshift);
LEPT_DLL extern L_RECOG * recogCreateFromPixa(PIXA * pixa,
    int32 scalew,
    int32 scaleh,
    int32 linew,
    int32 threshold,
    int32 maxyshift);
LEPT_DLL extern L_RECOG * recogCreateFromPixaNoFinish(PIXA * pixa,
    int32 scalew,
    int32 scaleh,
    int32 linew,
    int32 threshold,
    int32 maxyshift);
LEPT_DLL extern L_RECOG * recogCreate(int32 scalew, int32 scaleh, int32 linew, int32 threshold, int32 maxyshift);
LEPT_DLL extern void recogDestroy(L_RECOG ** precog);
LEPT_DLL extern int32 recogGetCount(L_RECOG * recog);
LEPT_DLL extern l_ok recogSetParams(L_RECOG * recog, int32 type, int32 min_nopad, float max_wh_ratio, float max_ht_ratio);
LEPT_DLL extern int32 recogGetClassIndex(L_RECOG * recog, int32 val, char * text, int32 * pindex);
LEPT_DLL extern l_ok recogStringToIndex(L_RECOG * recog, char * text, int32 * pindex);
LEPT_DLL extern int32 recogGetClassString(L_RECOG * recog, int32 index, char ** pcharstr);
LEPT_DLL extern l_ok l_convertCharstrToInt(const char * str, int32 * pval);
LEPT_DLL extern L_RECOG * recogRead(const char * filename);
LEPT_DLL extern L_RECOG * recogReadStream(FILE * fp);
LEPT_DLL extern L_RECOG * recogReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok recogWrite(const char * filename, L_RECOG * recog);
LEPT_DLL extern l_ok recogWriteStream(FILE * fp, L_RECOG * recog);
LEPT_DLL extern l_ok recogWriteMem(uint8 ** pdata, size_t * psize, L_RECOG * recog);
LEPT_DLL extern PIXA * recogExtractPixa(L_RECOG * recog);
LEPT_DLL extern BOXA * recogDecode(L_RECOG * recog, PIX * pixs, int32 nlevels, PIX ** ppixdb);
LEPT_DLL extern l_ok recogCreateDid(L_RECOG * recog, PIX * pixs);
LEPT_DLL extern l_ok recogDestroyDid(L_RECOG * recog);
LEPT_DLL extern int32 recogDidExists(L_RECOG * recog);
LEPT_DLL extern L_RDID * recogGetDid(L_RECOG * recog);
LEPT_DLL extern l_ok recogSetChannelParams(L_RECOG * recog, int32 nlevels);
LEPT_DLL extern l_ok recogIdentifyMultiple(L_RECOG * recog, PIX * pixs, int32 minh, int32 skipsplit, BOXA ** pboxa, PIXA ** ppixa, PIX ** ppixdb, int32 debugsplit);
LEPT_DLL extern l_ok recogSplitIntoCharacters(L_RECOG * recog, PIX * pixs, int32 minh, int32 skipsplit, BOXA ** pboxa, PIXA ** ppixa, int32 debug);
LEPT_DLL extern l_ok recogCorrelationBestRow(L_RECOG * recog,
    PIX * pixs,
    BOXA ** pboxa,
    NUMA ** pnascore,
    NUMA ** pnaindex,
    SARRAY ** psachar,
    int32 debug);
LEPT_DLL extern l_ok recogCorrelationBestChar(L_RECOG * recog,
    PIX * pixs,
    BOX ** pbox,
    float * pscore,
    int32 * pindex,
    char ** pcharstr,
    PIX ** ppixdb);
LEPT_DLL extern l_ok recogIdentifyPixa(L_RECOG * recog, PIXA * pixa, PIX ** ppixdb);
LEPT_DLL extern l_ok recogIdentifyPix(L_RECOG * recog, PIX * pixs, PIX ** ppixdb);
LEPT_DLL extern l_ok recogSkipIdentify(L_RECOG * recog);
LEPT_DLL extern void rchaDestroy(L_RCHA ** prcha);
LEPT_DLL extern void rchDestroy(L_RCH ** prch);
LEPT_DLL extern l_ok rchaExtract(L_RCHA * rcha,
    NUMA ** pnaindex,
    NUMA ** pnascore,
    SARRAY ** psatext,
    NUMA ** pnasample,
    NUMA ** pnaxloc,
    NUMA ** pnayloc,
    NUMA ** pnawidth);
LEPT_DLL extern l_ok rchExtract(L_RCH * rch,
    int32 * pindex,
    float * pscore,
    char ** ptext,
    int32 * psample,
    int32 * pxloc,
    int32 * pyloc,
    int32 * pwidth);
LEPT_DLL extern PIX * recogProcessToIdentify(L_RECOG * recog, PIX * pixs, int32 pad);
LEPT_DLL extern SARRAY * recogExtractNumbers(L_RECOG * recog,
    BOXA * boxas,
    float scorethresh,
    int32 spacethresh,
    BOXAA ** pbaa,
    NUMAA ** pnaa);
LEPT_DLL extern PIXA * showExtractNumbers(PIX * pixs, SARRAY * sa, BOXAA * baa, NUMAA * naa, PIX ** ppixdb);
LEPT_DLL extern l_ok recogTrainLabeled(L_RECOG * recog, PIX * pixs, BOX * box, char * text, int32 debug);
LEPT_DLL extern l_ok recogProcessLabeled(L_RECOG * recog, PIX * pixs, BOX * box, char * text, PIX ** ppix);
LEPT_DLL extern l_ok recogAddSample(L_RECOG * recog, PIX * pix, int32 debug);
LEPT_DLL extern PIX * recogModifyTemplate(L_RECOG * recog, PIX * pixs);
LEPT_DLL extern int32 recogAverageSamples(L_RECOG ** precog, int32 debug);
LEPT_DLL extern int32 pixaAccumulateSamples(PIXA * pixa, PTA * pta, PIX ** ppixd, float * px, float * py);
LEPT_DLL extern l_ok recogTrainingFinished(L_RECOG ** precog, int32 modifyflag, int32 minsize, float minfract);
LEPT_DLL extern PIXA * recogFilterPixaBySize(PIXA * pixas, int32 setsize, int32 maxkeep, float max_ht_ratio, NUMA ** pna);
LEPT_DLL extern PIXAA * recogSortPixaByClass(PIXA * pixa, int32 setsize);
LEPT_DLL extern l_ok recogRemoveOutliers1(L_RECOG ** precog, float minscore, int32 mintarget, int32 minsize, PIX ** ppixsave, PIX ** ppixrem);
LEPT_DLL extern PIXA * pixaRemoveOutliers1(PIXA * pixas, float minscore, int32 mintarget, int32 minsize, PIX ** ppixsave, PIX ** ppixrem);
LEPT_DLL extern l_ok recogRemoveOutliers2(L_RECOG ** precog, float minscore, int32 minsize, PIX ** ppixsave, PIX ** ppixrem);
LEPT_DLL extern PIXA * pixaRemoveOutliers2(PIXA * pixas, float minscore, int32 minsize, PIX ** ppixsave, PIX ** ppixrem);
LEPT_DLL extern PIXA * recogTrainFromBoot(L_RECOG * recogboot, PIXA * pixas, float minscore, int32 threshold, int32 debug);
LEPT_DLL extern l_ok recogPadDigitTrainingSet(L_RECOG ** precog, int32 scaleh, int32 linew);
LEPT_DLL extern int32 recogIsPaddingNeeded(L_RECOG * recog, SARRAY ** psa);
LEPT_DLL extern PIXA * recogAddDigitPadTemplates(L_RECOG * recog, SARRAY * sa);
LEPT_DLL extern L_RECOG * recogMakeBootDigitRecog(int32 nsamp, int32 scaleh, int32 linew, int32 maxyshift, int32 debug);
LEPT_DLL extern PIXA * recogMakeBootDigitTemplates(int32 nsamp, int32 debug);
LEPT_DLL extern l_ok recogShowContent(FILE * fp, L_RECOG * recog, int32 index, int32 display);
LEPT_DLL extern l_ok recogDebugAverages(L_RECOG ** precog, int32 debug);
LEPT_DLL extern int32 recogShowAverageTemplates(L_RECOG * recog);
LEPT_DLL extern l_ok recogShowMatchesInRange(L_RECOG * recog, PIXA * pixa, float minscore, float maxscore, int32 display);
LEPT_DLL extern PIX * recogShowMatch(L_RECOG * recog, PIX * pix1, PIX * pix2, BOX * box, int32 index, float score);
LEPT_DLL extern l_ok regTestSetup(int32 argc, char ** argv, L_REGPARAMS ** prp);
LEPT_DLL extern l_ok regTestCleanup(L_REGPARAMS * rp);
LEPT_DLL extern l_ok regTestCompareValues(L_REGPARAMS * rp, float val1, float val2, float delta);
LEPT_DLL extern l_ok regTestCompareStrings(L_REGPARAMS * rp, uint8 * string1, size_t bytes1, uint8 * string2, size_t bytes2);
LEPT_DLL extern l_ok regTestComparePix(L_REGPARAMS * rp, PIX * pix1, PIX * pix2);
LEPT_DLL extern l_ok regTestCompareSimilarPix(L_REGPARAMS * rp, PIX * pix1, PIX * pix2, int32 mindiff, float maxfract, int32 printstats);
LEPT_DLL extern l_ok regTestCheckFile(L_REGPARAMS * rp, const char * localname);
LEPT_DLL extern l_ok regTestCompareFiles(L_REGPARAMS * rp, int32 index1, int32 index2);
LEPT_DLL extern l_ok regTestWritePixAndCheck(L_REGPARAMS * rp, PIX * pix, int32 format);
LEPT_DLL extern l_ok regTestWriteDataAndCheck(L_REGPARAMS * rp, void * data, size_t nbytes, const char * ext);
LEPT_DLL extern char * regTestGenLocalFilename(L_REGPARAMS * rp, int32 index, int32 format);
LEPT_DLL extern l_ok pixRasterop(PIX * pixd, int32 dx, int32 dy, int32 dw, int32 dh, int32 op, PIX * pixs, int32 sx, int32 sy);
LEPT_DLL extern l_ok pixRasteropVip(PIX * pixd, int32 bx, int32 bw, int32 vshift, int32 incolor);
LEPT_DLL extern l_ok pixRasteropHip(PIX * pixd, int32 by, int32 bh, int32 hshift, int32 incolor);
LEPT_DLL extern PIX * pixTranslate(PIX * pixd, PIX * pixs, int32 hshift, int32 vshift, int32 incolor);
LEPT_DLL extern l_ok pixRasteropIP(PIX * pixd, int32 hshift, int32 vshift, int32 incolor);
LEPT_DLL extern l_ok pixRasteropFullImage(PIX * pixd, PIX * pixs, int32 op);
LEPT_DLL extern void rasteropUniLow(uint32 * datad, int32 dpixw, int32 dpixh, int32 depth, int32 dwpl, int32 dx, int32 dy, int32 dw, int32 dh, int32 op);
LEPT_DLL extern void rasteropLow(uint32 * datad, int32 dpixw, int32 dpixh, int32 depth, int32 dwpl, int32 dx, int32 dy, int32 dw, int32 dh,
    int32 op, uint32 * datas, int32 spixw, int32 spixh, int32 swpl, int32 sx, int32 sy);
LEPT_DLL extern void rasteropVipLow(uint32 * data, int32 pixw, int32 pixh, int32 depth, int32 wpl, int32 x, int32 w, int32 shift);
LEPT_DLL extern void rasteropHipLow(uint32 * data, int32 pixh, int32 depth, int32 wpl, int32 y, int32 h, int32 shift);
LEPT_DLL extern PIX * pixRotate(PIX * pixs, float angle, int32 type, int32 incolor, int32 width, int32 height);
LEPT_DLL extern PIX * pixEmbedForRotation(PIX * pixs, float angle, int32 incolor, int32 width, int32 height);
LEPT_DLL extern PIX * pixRotateBySampling(PIX * pixs, int32 xcen, int32 ycen, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotateBinaryNice(PIX * pixs, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotateWithAlpha(PIX * pixs, float angle, PIX * pixg, float fract);
LEPT_DLL extern PIX * pixRotateAM(PIX * pixs, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotateAMColor(PIX * pixs, float angle, uint32 colorval);
LEPT_DLL extern PIX * pixRotateAMGray(PIX * pixs, float angle, uint8 grayval);
LEPT_DLL extern PIX * pixRotateAMCorner(PIX * pixs, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotateAMColorCorner(PIX * pixs, float angle, uint32 fillval);
LEPT_DLL extern PIX * pixRotateAMGrayCorner(PIX * pixs, float angle, uint8 grayval);
LEPT_DLL extern PIX * pixRotateAMColorFast(PIX * pixs, float angle, uint32 colorval);
LEPT_DLL extern PIX * pixRotateOrth(PIX * pixs, int32 quads);
LEPT_DLL extern PIX * pixRotate180(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixRotate90(PIX * pixs, int32 direction);
LEPT_DLL extern PIX * pixFlipLR(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixFlipTB(PIX * pixd, PIX * pixs);
LEPT_DLL extern PIX * pixRotateShear(PIX * pixs, int32 xcen, int32 ycen, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotate2Shear(PIX * pixs, int32 xcen, int32 ycen, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotate3Shear(PIX * pixs, int32 xcen, int32 ycen, float angle, int32 incolor);
LEPT_DLL extern l_ok pixRotateShearIP(PIX * pixs, int32 xcen, int32 ycen, float angle, int32 incolor);
LEPT_DLL extern PIX * pixRotateShearCenter(PIX * pixs, float angle, int32 incolor);
LEPT_DLL extern l_ok pixRotateShearCenterIP(PIX * pixs, float angle, int32 incolor);
LEPT_DLL extern PIX * pixStrokeWidthTransform(PIX * pixs, int32 color, int32 depth, int32 nangles);
LEPT_DLL extern PIX * pixRunlengthTransform(PIX * pixs, int32 color, int32 direction, int32 depth);
LEPT_DLL extern l_ok pixFindHorizontalRuns(PIX * pix, int32 y, int32 * xstart, int32 * xend, int32 * pn);
LEPT_DLL extern l_ok pixFindVerticalRuns(PIX * pix, int32 x, int32 * ystart, int32 * yend, int32 * pn);
LEPT_DLL extern NUMA * pixFindMaxRuns(PIX * pix, int32 direction, NUMA ** pnastart);
LEPT_DLL extern l_ok pixFindMaxHorizontalRunOnLine(PIX * pix, int32 y, int32 * pxstart, int32 * psize);
LEPT_DLL extern l_ok pixFindMaxVerticalRunOnLine(PIX * pix, int32 x, int32 * pystart, int32 * psize);
LEPT_DLL extern l_ok runlengthMembershipOnLine(int32 * buffer, int32 size, int32 depth, int32 * start, int32 * end, int32 n);
LEPT_DLL extern int32 * makeMSBitLocTab(int32 bitval);
LEPT_DLL extern SARRAY * sarrayCreate(int32 n);
LEPT_DLL extern SARRAY * sarrayCreateInitialized(int32 n, const char * initstr);
LEPT_DLL extern SARRAY * sarrayCreateWordsFromString(const char * string);
LEPT_DLL extern SARRAY * sarrayCreateLinesFromString(const char * string, int32 blankflag);
LEPT_DLL extern void sarrayDestroy(SARRAY ** psa);
LEPT_DLL extern SARRAY * sarrayCopy(SARRAY * sa);
LEPT_DLL extern SARRAY * sarrayClone(SARRAY * sa);
LEPT_DLL extern l_ok sarrayAddString(SARRAY * sa, const char * string, int32 copyflag);
LEPT_DLL extern char * sarrayRemoveString(SARRAY * sa, int32 index);
LEPT_DLL extern l_ok sarrayReplaceString(SARRAY * sa, int32 index, char * newstr, int32 copyflag);
LEPT_DLL extern l_ok sarrayClear(SARRAY * sa);
LEPT_DLL extern int32 sarrayGetCount(SARRAY * sa);
LEPT_DLL extern char ** sarrayGetArray(SARRAY * sa, int32 * pnalloc, int32 * pn);
LEPT_DLL extern char * sarrayGetString(SARRAY * sa, int32 index, int32 copyflag);
LEPT_DLL extern int32 sarrayGetRefcount(SARRAY * sa);
LEPT_DLL extern l_ok sarrayChangeRefcount(SARRAY * sa, int32 delta);
LEPT_DLL extern char * sarrayToString(SARRAY * sa, int32 addnlflag);
LEPT_DLL extern char * sarrayToStringRange(SARRAY * sa, int32 first, int32 nstrings, int32 addnlflag);
LEPT_DLL extern SARRAY * sarrayConcatUniformly(SARRAY * sa, int32 n, int32 addnlflag);
LEPT_DLL extern l_ok sarrayJoin(SARRAY * sa1, SARRAY * sa2);
LEPT_DLL extern l_ok sarrayAppendRange(SARRAY * sa1, SARRAY * sa2, int32 start, int32 end);
LEPT_DLL extern l_ok sarrayPadToSameSize(SARRAY * sa1, SARRAY * sa2, const char * padstring);
LEPT_DLL extern SARRAY * sarrayConvertWordsToLines(SARRAY * sa, int32 linesize);
LEPT_DLL extern int32 sarraySplitString(SARRAY * sa, const char * str, const char * separators);
LEPT_DLL extern SARRAY * sarraySelectBySubstring(SARRAY * sain, const char * substr);
LEPT_DLL extern SARRAY * sarraySelectRange(SARRAY * sain, int32 first, int32 last);
LEPT_DLL extern int32 sarrayParseRange(SARRAY * sa, int32 start, int32 * pactualstart, int32 * pend, int32 * pnewstart, const char * substr, int32 loc);
LEPT_DLL extern SARRAY * sarrayRead(const char * filename);
LEPT_DLL extern SARRAY * sarrayReadStream(FILE * fp);
LEPT_DLL extern SARRAY * sarrayReadMem(const uint8 * data, size_t size);
LEPT_DLL extern l_ok sarrayWrite(const char * filename, SARRAY * sa);
LEPT_DLL extern l_ok sarrayWriteStream(FILE * fp, SARRAY * sa);
LEPT_DLL extern l_ok sarrayWriteStderr(SARRAY * sa);
LEPT_DLL extern l_ok sarrayWriteMem(uint8 ** pdata, size_t * psize, SARRAY * sa);
LEPT_DLL extern l_ok sarrayAppend(const char * filename, SARRAY * sa);
LEPT_DLL extern SARRAY * getNumberedPathnamesInDirectory(const char * dirname, const char * substr, int32 numpre, int32 numpost, int32 maxnum);
LEPT_DLL extern SARRAY * getSortedPathnamesInDirectory(const char * dirname, const char * substr, int32 first, int32 nfiles);
LEPT_DLL extern SARRAY * convertSortedToNumberedPathnames(SARRAY * sa, int32 numpre, int32 numpost, int32 maxnum);
LEPT_DLL extern SARRAY * getFilenamesInDirectory(const char * dirname);
LEPT_DLL extern SARRAY * sarraySort(SARRAY * saout, SARRAY * sain, int32 sortorder);
LEPT_DLL extern SARRAY * sarraySortByIndex(SARRAY * sain, NUMA * naindex);
LEPT_DLL extern int32 stringCompareLexical(const char * str1, const char * str2);
LEPT_DLL extern L_ASET * l_asetCreateFromSarray(SARRAY * sa);
LEPT_DLL extern l_ok sarrayRemoveDupsByAset(SARRAY * sas, SARRAY ** psad);
LEPT_DLL extern l_ok sarrayUnionByAset(SARRAY * sa1, SARRAY * sa2, SARRAY ** psad);
LEPT_DLL extern l_ok sarrayIntersectionByAset(SARRAY * sa1, SARRAY * sa2, SARRAY ** psad);
LEPT_DLL extern L_HASHMAP * l_hmapCreateFromSarray(SARRAY * sa);
LEPT_DLL extern l_ok sarrayRemoveDupsByHmap(SARRAY * sas, SARRAY ** psad, L_HASHMAP ** phmap);
LEPT_DLL extern l_ok sarrayUnionByHmap(SARRAY * sa1, SARRAY * sa2, SARRAY ** psad);
LEPT_DLL extern l_ok sarrayIntersectionByHmap(SARRAY * sa1, SARRAY * sa2, SARRAY ** psad);
LEPT_DLL extern SARRAY * sarrayGenerateIntegers(int32 n);
LEPT_DLL extern l_ok sarrayLookupCSKV(SARRAY * sa, const char * keystring, char ** pvalstring);
LEPT_DLL extern PIX * pixScale(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleToSizeRel(PIX * pixs, int32 delw, int32 delh);
LEPT_DLL extern PIX * pixScaleToSize(PIX * pixs, int32 wd, int32 hd);
LEPT_DLL extern PIX * pixScaleToResolution(PIX * pixs, float target, float assumed, float * pscalefact);
LEPT_DLL extern PIX * pixScaleGeneral(PIX * pixs, float scalex, float scaley, float sharpfract, int32 sharpwidth);
LEPT_DLL extern PIX * pixScaleLI(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleColorLI(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleColor2xLI(PIX * pixs);
LEPT_DLL extern PIX * pixScaleColor4xLI(PIX * pixs);
LEPT_DLL extern PIX * pixScaleGrayLI(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleGray2xLI(PIX * pixs);
LEPT_DLL extern PIX * pixScaleGray4xLI(PIX * pixs);
LEPT_DLL extern PIX * pixScaleGray2xLIThresh(PIX * pixs, int32 thresh);
LEPT_DLL extern PIX * pixScaleGray2xLIDither(PIX * pixs);
LEPT_DLL extern PIX * pixScaleGray4xLIThresh(PIX * pixs, int32 thresh);
LEPT_DLL extern PIX * pixScaleGray4xLIDither(PIX * pixs);
LEPT_DLL extern PIX * pixScaleBySampling(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleBySamplingToSize(PIX * pixs, int32 wd, int32 hd);
LEPT_DLL extern PIX * pixScaleByIntSampling(PIX * pixs, int32 factor);
LEPT_DLL extern PIX * pixScaleRGBToGrayFast(PIX * pixs, int32 factor, int32 color);
LEPT_DLL extern PIX * pixScaleRGBToBinaryFast(PIX * pixs, int32 factor, int32 thresh);
LEPT_DLL extern PIX * pixScaleGrayToBinaryFast(PIX * pixs, int32 factor, int32 thresh);
LEPT_DLL extern PIX * pixScaleSmooth(PIX * pix, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleSmoothToSize(PIX * pixs, int32 wd, int32 hd);
LEPT_DLL extern PIX * pixScaleRGBToGray2(PIX * pixs, float rwt, float gwt, float bwt);
LEPT_DLL extern PIX * pixScaleAreaMap(PIX * pix, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleAreaMap2(PIX * pix);
LEPT_DLL extern PIX * pixScaleAreaMapToSize(PIX * pixs, int32 wd, int32 hd);
LEPT_DLL extern PIX * pixScaleBinary(PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleToGray(PIX * pixs, float scalefactor);
LEPT_DLL extern PIX * pixScaleToGrayFast(PIX * pixs, float scalefactor);
LEPT_DLL extern PIX * pixScaleToGray2(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGray3(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGray4(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGray6(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGray8(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGray16(PIX * pixs);
LEPT_DLL extern PIX * pixScaleToGrayMipmap(PIX * pixs, float scalefactor);
LEPT_DLL extern PIX * pixScaleMipmap(PIX * pixs1, PIX * pixs2, float scale);
LEPT_DLL extern PIX * pixExpandReplicate(PIX * pixs, int32 factor);
LEPT_DLL extern PIX * pixScaleGrayMinMax(PIX * pixs, int32 xfact, int32 yfact, int32 type);
LEPT_DLL extern PIX * pixScaleGrayMinMax2(PIX * pixs, int32 type);
LEPT_DLL extern PIX * pixScaleGrayRankCascade(PIX * pixs, int32 level1, int32 level2, int32 level3, int32 level4);
LEPT_DLL extern PIX * pixScaleGrayRank2(PIX * pixs, int32 rank);
LEPT_DLL extern l_ok pixScaleAndTransferAlpha(PIX * pixd, PIX * pixs, float scalex, float scaley);
LEPT_DLL extern PIX * pixScaleWithAlpha(PIX * pixs, float scalex, float scaley, PIX * pixg, float fract);
LEPT_DLL extern PIX * pixSeedfillBinary(PIX * pixd, PIX * pixs, PIX * pixm, int32 connectivity);
LEPT_DLL extern PIX * pixSeedfillBinaryRestricted(PIX * pixd, PIX * pixs, PIX * pixm, int32 connectivity, int32 xmax, int32 ymax);
LEPT_DLL extern PIX * pixHolesByFilling(PIX * pixs, int32 connectivity);
LEPT_DLL extern PIX * pixFillClosedBorders(PIX * pixs, int32 connectivity);
LEPT_DLL extern PIX * pixExtractBorderConnComps(PIX * pixs, int32 connectivity);
LEPT_DLL extern PIX * pixRemoveBorderConnComps(PIX * pixs, int32 connectivity);
LEPT_DLL extern PIX * pixFillBgFromBorder(PIX * pixs, int32 connectivity);
LEPT_DLL extern PIX * pixFillHolesToBoundingRect(PIX * pixs, int32 minsize, float maxhfract, float minfgfract);
LEPT_DLL extern l_ok pixSeedfillGray(PIX * pixs, PIX * pixm, int32 connectivity);
LEPT_DLL extern l_ok pixSeedfillGrayInv(PIX * pixs, PIX * pixm, int32 connectivity);
LEPT_DLL extern l_ok pixSeedfillGraySimple(PIX * pixs, PIX * pixm, int32 connectivity);
LEPT_DLL extern l_ok pixSeedfillGrayInvSimple(PIX * pixs, PIX * pixm, int32 connectivity);
LEPT_DLL extern PIX * pixSeedfillGrayBasin(PIX * pixb, PIX * pixm, int32 delta, int32 connectivity);
LEPT_DLL extern PIX * pixDistanceFunction(PIX * pixs, int32 connectivity, int32 outdepth, int32 boundcond);
LEPT_DLL extern PIX * pixSeedspread(PIX * pixs, int32 connectivity);
LEPT_DLL extern l_ok pixLocalExtrema(PIX * pixs, int32 maxmin, int32 minmax, PIX ** ppixmin, PIX ** ppixmax);
LEPT_DLL extern l_ok pixSelectedLocalExtrema(PIX * pixs, int32 mindist, PIX ** ppixmin, PIX ** ppixmax);
LEPT_DLL extern PIX * pixFindEqualValues(PIX * pixs1, PIX * pixs2);
LEPT_DLL extern l_ok pixSelectMinInConnComp(PIX * pixs, PIX * pixm, PTA ** ppta, NUMA ** pnav);
LEPT_DLL extern PIX * pixRemoveSeededComponents(PIX * pixd, PIX * pixs, PIX * pixm, int32 connectivity, int32 bordersize);
LEPT_DLL extern SELA * selaCreate(int32 n);
LEPT_DLL extern void selaDestroy(SELA ** psela);
LEPT_DLL extern SEL * selCreate(int32 height, int32 width, const char * name);
LEPT_DLL extern void selDestroy(SEL ** psel);
LEPT_DLL extern SEL * selCopy(SEL * sel);
LEPT_DLL extern SEL * selCreateBrick(int32 h, int32 w, int32 cy, int32 cx, int32 type);
LEPT_DLL extern SEL * selCreateComb(int32 factor1, int32 factor2, int32 direction);
LEPT_DLL extern int32 ** create2dIntArray(int32 sy, int32 sx);
LEPT_DLL extern l_ok selaAddSel(SELA * sela, SEL * sel, const char * selname, int32 copyflag);
LEPT_DLL extern int32 selaGetCount(SELA * sela);
LEPT_DLL extern SEL * selaGetSel(SELA * sela, int32 i);
LEPT_DLL extern char * selGetName(SEL * sel);
LEPT_DLL extern l_ok selSetName(SEL * sel, const char * name);
LEPT_DLL extern l_ok selaFindSelByName(SELA * sela, const char * name, int32 * pindex, SEL ** psel);
LEPT_DLL extern l_ok selGetElement(SEL * sel, int32 row, int32 col, int32 * ptype);
LEPT_DLL extern l_ok selSetElement(SEL * sel, int32 row, int32 col, int32 type);
LEPT_DLL extern l_ok selGetParameters(SEL * sel, int32 * psy, int32 * psx, int32 * pcy, int32 * pcx);
LEPT_DLL extern l_ok selSetOrigin(SEL * sel, int32 cy, int32 cx);
LEPT_DLL extern l_ok selGetTypeAtOrigin(SEL * sel, int32 * ptype);
LEPT_DLL extern char * selaGetBrickName(SELA * sela, int32 hsize, int32 vsize);
LEPT_DLL extern char * selaGetCombName(SELA * sela, int32 size, int32 direction);
LEPT_DLL extern l_ok getCompositeParameters(int32 size, int32 * psize1, int32 * psize2, char ** pnameh1, char ** pnameh2, char ** pnamev1, char ** pnamev2);
LEPT_DLL extern SARRAY * selaGetSelnames(SELA * sela);
LEPT_DLL extern l_ok selFindMaxTranslations(SEL * sel, int32 * pxp, int32 * pyp, int32 * pxn, int32 * pyn);
LEPT_DLL extern SEL * selRotateOrth(SEL * sel, int32 quads);
LEPT_DLL extern SELA * selaRead(const char * fname);
LEPT_DLL extern SELA * selaReadStream(FILE * fp);
LEPT_DLL extern SEL * selRead(const char * fname);
LEPT_DLL extern SEL * selReadStream(FILE * fp);
LEPT_DLL extern l_ok selaWrite(const char * fname, SELA * sela);
LEPT_DLL extern l_ok selaWriteStream(FILE * fp, SELA * sela);
LEPT_DLL extern l_ok selWrite(const char * fname, SEL * sel);
LEPT_DLL extern l_ok selWriteStream(FILE * fp, SEL * sel);
LEPT_DLL extern SEL * selCreateFromString(const char * text, int32 h, int32 w, const char * name);
LEPT_DLL extern char * selPrintToString(SEL * sel);
LEPT_DLL extern SELA * selaCreateFromFile(const char * filename);
LEPT_DLL extern SEL * selCreateFromPta(PTA * pta, int32 cy, int32 cx, const char * name);
LEPT_DLL extern SEL * selCreateFromPix(PIX * pix, int32 cy, int32 cx, const char * name);
LEPT_DLL extern SEL * selReadFromColorImage(const char * pathname);
LEPT_DLL extern SEL * selCreateFromColorPix(PIX * pixs, const char * selname);
LEPT_DLL extern SELA * selaCreateFromColorPixa(PIXA * pixa, SARRAY * sa);
LEPT_DLL extern PIX * selDisplayInPix(SEL * sel, int32 size, int32 gthick);
LEPT_DLL extern PIX * selaDisplayInPix(SELA * sela, int32 size, int32 gthick, int32 spacing, int32 ncols);
LEPT_DLL extern SELA * selaAddBasic(SELA * sela);
LEPT_DLL extern SELA * selaAddHitMiss(SELA * sela);
LEPT_DLL extern SELA * selaAddDwaLinear(SELA * sela);
LEPT_DLL extern SELA * selaAddDwaCombs(SELA * sela);
LEPT_DLL extern SELA * selaAddCrossJunctions(SELA * sela, float hlsize, float mdist, int32 norient, int32 debugflag);
LEPT_DLL extern SELA * selaAddTJunctions(SELA * sela, float hlsize, float mdist, int32 norient, int32 debugflag);
LEPT_DLL extern SELA * sela4ccThin(SELA * sela);
LEPT_DLL extern SELA * sela8ccThin(SELA * sela);
LEPT_DLL extern SELA * sela4and8ccThin(SELA * sela);
LEPT_DLL extern SEL * selMakePlusSign(int32 size, int32 linewidth);
LEPT_DLL extern SEL * pixGenerateSelWithRuns(PIX * pixs,
    int32 nhlines,
    int32 nvlines,
    int32 distance,
    int32 minlength,
    int32 toppix,
    int32 botpix,
    int32 leftpix,
    int32 rightpix,
    PIX ** ppixe);
LEPT_DLL extern SEL * pixGenerateSelRandom(PIX * pixs,
    float hitfract,
    float missfract,
    int32 distance,
    int32 toppix,
    int32 botpix,
    int32 leftpix,
    int32 rightpix,
    PIX ** ppixe);
LEPT_DLL extern SEL * pixGenerateSelBoundary(PIX * pixs,
    int32 hitdist,
    int32 missdist,
    int32 hitskip,
    int32 missskip,
    int32 topflag,
    int32 botflag,
    int32 leftflag,
    int32 rightflag,
    PIX ** ppixe);
LEPT_DLL extern NUMA * pixGetRunCentersOnLine(PIX * pixs, int32 x, int32 y, int32 minlength);
LEPT_DLL extern NUMA * pixGetRunsOnLine(PIX * pixs, int32 x1, int32 y1, int32 x2, int32 y2);
LEPT_DLL extern PTA * pixSubsampleBoundaryPixels(PIX * pixs, int32 skip);
LEPT_DLL extern int32 adjacentOnPixelInRaster(PIX * pixs, int32 x, int32 y, int32 * pxa, int32 * pya);
LEPT_DLL extern PIX * pixDisplayHitMissSel(PIX * pixs, SEL * sel, int32 scalefactor, uint32 hitcolor, uint32 misscolor);
LEPT_DLL extern PIX * pixHShear(PIX * pixd, PIX * pixs, int32 yloc, float radang, int32 incolor);
LEPT_DLL extern PIX * pixVShear(PIX * pixd, PIX * pixs, int32 xloc, float radang, int32 incolor);
LEPT_DLL extern PIX * pixHShearCorner(PIX * pixd, PIX * pixs, float radang, int32 incolor);
LEPT_DLL extern PIX * pixVShearCorner(PIX * pixd, PIX * pixs, float radang, int32 incolor);
LEPT_DLL extern PIX * pixHShearCenter(PIX * pixd, PIX * pixs, float radang, int32 incolor);
LEPT_DLL extern PIX * pixVShearCenter(PIX * pixd, PIX * pixs, float radang, int32 incolor);
LEPT_DLL extern l_ok pixHShearIP(PIX * pixs, int32 yloc, float radang, int32 incolor);
LEPT_DLL extern l_ok pixVShearIP(PIX * pixs, int32 xloc, float radang, int32 incolor);
LEPT_DLL extern PIX * pixHShearLI(PIX * pixs, int32 yloc, float radang, int32 incolor);
LEPT_DLL extern PIX * pixVShearLI(PIX * pixs, int32 xloc, float radang, int32 incolor);
LEPT_DLL extern PIX * pixDeskewBoth(PIX * pixs, int32 redsearch);
LEPT_DLL extern PIX * pixDeskew(PIX * pixs, int32 redsearch);
LEPT_DLL extern PIX * pixFindSkewAndDeskew(PIX * pixs, int32 redsearch, float * pangle, float * pconf);
LEPT_DLL extern PIX * pixDeskewGeneral(PIX * pixs,
    int32 redsweep,
    float sweeprange,
    float sweepdelta,
    int32 redsearch,
    int32 thresh,
    float * pangle,
    float * pconf);
LEPT_DLL extern l_ok pixFindSkew(PIX * pixs, float * pangle, float * pconf);
LEPT_DLL extern l_ok pixFindSkewSweep(PIX * pixs, float * pangle, int32 reduction, float sweeprange, float sweepdelta);
LEPT_DLL extern l_ok pixFindSkewSweepAndSearch(PIX * pixs,
    float * pangle,
    float * pconf,
    int32 redsweep,
    int32 redsearch,
    float sweeprange,
    float sweepdelta,
    float minbsdelta);
LEPT_DLL extern l_ok pixFindSkewSweepAndSearchScore(PIX * pixs,
    float * pangle,
    float * pconf,
    float * pendscore,
    int32 redsweep,
    int32 redsearch,
    float sweepcenter,
    float sweeprange,
    float sweepdelta,
    float minbsdelta);
LEPT_DLL extern l_ok pixFindSkewSweepAndSearchScorePivot(PIX * pixs,
    float * pangle,
    float * pconf,
    float * pendscore,
    int32 redsweep,
    int32 redsearch,
    float sweepcenter,
    float sweeprange,
    float sweepdelta,
    float minbsdelta,
    int32 pivot);
LEPT_DLL extern int32 pixFindSkewOrthogonalRange(PIX * pixs, float * pangle, float * pconf, int32 redsweep, int32 redsearch, float sweeprange, float sweepdelta, float minbsdelta, float confprior);
LEPT_DLL extern l_ok pixFindDifferentialSquareSum(PIX * pixs, float * psum);
LEPT_DLL extern l_ok pixFindNormalizedSquareSum(PIX * pixs, float * phratio, float * pvratio, float * pfract);
LEPT_DLL extern PIX * pixReadStreamSpix(FILE * fp);
LEPT_DLL extern l_ok readHeaderSpix(const char * filename, int32 * pwidth, int32 * pheight, int32 * pbps, int32 * pspp, int32 * piscmap);
LEPT_DLL extern l_ok freadHeaderSpix(FILE * fp, int32 * pwidth, int32 * pheight, int32 * pbps, int32 * pspp, int32 * piscmap);
LEPT_DLL extern l_ok sreadHeaderSpix(const uint32 * data, size_t size, int32 * pwidth, int32 * pheight, int32 * pbps, int32 * pspp, int32 * piscmap);
LEPT_DLL extern l_ok pixWriteStreamSpix(FILE * fp, PIX * pix);
LEPT_DLL extern PIX * pixReadMemSpix(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixWriteMemSpix(uint8 ** pdata, size_t * psize, PIX * pix);
LEPT_DLL extern l_ok pixSerializeToMemory(PIX * pixs, uint32 ** pdata, size_t * pnbytes);
LEPT_DLL extern PIX * pixDeserializeFromMemory(const uint32 * data, size_t nbytes);
LEPT_DLL extern L_STACK * lstackCreate(int32 n);
LEPT_DLL extern void lstackDestroy(L_STACK ** plstack, int32 freeflag);
LEPT_DLL extern l_ok lstackAdd(L_STACK * lstack, void * item);
LEPT_DLL extern void * lstackRemove(L_STACK * lstack);
LEPT_DLL extern int32 lstackGetCount(L_STACK * lstack);
LEPT_DLL extern l_ok lstackPrint(FILE * fp, L_STACK * lstack);
LEPT_DLL extern L_STRCODE * strcodeCreate(int32 fileno);
LEPT_DLL extern l_ok strcodeCreateFromFile(const char * filein, int32 fileno, const char * outdir);
LEPT_DLL extern l_ok strcodeGenerate(L_STRCODE * strcode, const char * filein, const char * type);
LEPT_DLL extern int32 strcodeFinalize(L_STRCODE ** pstrcode, const char * outdir);
LEPT_DLL extern int32 l_getStructStrFromFile(const char * filename, int32 field, char ** pstr);
LEPT_DLL extern l_ok pixFindStrokeLength(PIX * pixs, int32 * tab8, int32 * plength);
LEPT_DLL extern l_ok pixFindStrokeWidth(PIX * pixs, float thresh, int32 * tab8, float * pwidth, NUMA ** pnahisto);
LEPT_DLL extern NUMA * pixaFindStrokeWidth(PIXA * pixa, float thresh, int32 * tab8, int32 debug);
LEPT_DLL extern PIXA * pixaModifyStrokeWidth(PIXA * pixas, float targetw);
LEPT_DLL extern PIX * pixModifyStrokeWidth(PIX * pixs, float width, float targetw);
LEPT_DLL extern PIXA * pixaSetStrokeWidth(PIXA * pixas, int32 width, int32 thinfirst, int32 connectivity);
LEPT_DLL extern PIX * pixSetStrokeWidth(PIX * pixs, int32 width, int32 thinfirst, int32 connectivity);
LEPT_DLL extern int32 * sudokuReadFile(const char * filename);
LEPT_DLL extern int32 * sudokuReadString(const char * str);
LEPT_DLL extern L_SUDOKU * sudokuCreate(int32 * array);
LEPT_DLL extern void sudokuDestroy(L_SUDOKU ** psud);
LEPT_DLL extern int32 sudokuSolve(L_SUDOKU * sud);
LEPT_DLL extern l_ok sudokuTestUniqueness(int32 * array, int32 * punique);
LEPT_DLL extern L_SUDOKU * sudokuGenerate(int32 * array, int32 seed, int32 minelems, int32 maxtries);
LEPT_DLL extern int32 sudokuOutput(L_SUDOKU * sud, int32 arraytype);
LEPT_DLL extern PIX * pixAddSingleTextblock(PIX * pixs, L_BMF * bmf, const char * textstr, uint32 val, int32 location, int32 * poverflow);
LEPT_DLL extern PIX * pixAddTextlines(PIX * pixs, L_BMF * bmf, const char * textstr, uint32 val, int32 location);
LEPT_DLL extern l_ok pixSetTextblock(PIX * pixs, L_BMF * bmf, const char * textstr, uint32 val, int32 x0, int32 y0, int32 wtext, int32 firstindent, int32 * poverflow);
LEPT_DLL extern l_ok pixSetTextline(PIX * pixs, L_BMF * bmf, const char * textstr, uint32 val, int32 x0, int32 y0, int32 * pwidth, int32 * poverflow);
LEPT_DLL extern PIXA * pixaAddTextNumber(PIXA * pixas, L_BMF * bmf, NUMA * na, uint32 val, int32 location);
LEPT_DLL extern PIXA * pixaAddTextlines(PIXA * pixas, L_BMF * bmf, SARRAY * sa, uint32 val, int32 location);
LEPT_DLL extern l_ok pixaAddPixWithText(PIXA * pixa, PIX * pixs, int32 reduction, L_BMF * bmf, const char * textstr, uint32 val, int32 location);
LEPT_DLL extern SARRAY * bmfGetLineStrings(L_BMF * bmf, const char * textstr, int32 maxw, int32 firstindent, int32 * ph);
LEPT_DLL extern NUMA * bmfGetWordWidths(L_BMF * bmf, const char * textstr, SARRAY * sa);
LEPT_DLL extern l_ok bmfGetStringWidth(L_BMF * bmf, const char * textstr, int32 * pw);
LEPT_DLL extern SARRAY * splitStringToParagraphs(char * textstr, int32 splitflag);
LEPT_DLL extern PIX * pixReadTiff(const char * filename, int32 n);
LEPT_DLL extern PIX * pixReadStreamTiff(FILE * fp, int32 n);
LEPT_DLL extern l_ok pixWriteTiff(const char * filename, PIX * pix, int32 comptype, const char * modestr);
LEPT_DLL extern l_ok pixWriteTiffCustom(const char * filename, PIX * pix, int32 comptype, const char * modestr, NUMA * natags, SARRAY * savals, SARRAY * satypes, NUMA * nasizes);
LEPT_DLL extern l_ok pixWriteStreamTiff(FILE * fp, PIX * pix, int32 comptype);
LEPT_DLL extern l_ok pixWriteStreamTiffWA(FILE * fp, PIX * pix, int32 comptype, const char * modestr);
LEPT_DLL extern PIX * pixReadFromMultipageTiff(const char * fname, size_t * poffset);
LEPT_DLL extern PIXA * pixaReadMultipageTiff(const char * filename);
LEPT_DLL extern l_ok pixaWriteMultipageTiff(const char * fname, PIXA * pixa);
LEPT_DLL extern l_ok writeMultipageTiff(const char * dirin, const char * substr, const char * fileout);
LEPT_DLL extern l_ok writeMultipageTiffSA(SARRAY * sa, const char * fileout);
LEPT_DLL extern l_ok fprintTiffInfo(FILE * fpout, const char * tiffile);
LEPT_DLL extern l_ok tiffGetCount(FILE * fp, int32 * pn);
LEPT_DLL extern l_ok getTiffResolution(FILE * fp, int32 * pxres, int32 * pyres);
LEPT_DLL extern l_ok readHeaderTiff(const char * filename, int32 n, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pres, int32 * pcmap, int32 * pformat);
LEPT_DLL extern l_ok freadHeaderTiff(FILE * fp, int32 n, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pres, int32 * pcmap, int32 * pformat);
LEPT_DLL extern l_ok readHeaderMemTiff(const uint8 * cdata, size_t size, int32 n, int32 * pw, int32 * ph, int32 * pbps, int32 * pspp, int32 * pres, int32 * pcmap, int32 * pformat);
LEPT_DLL extern l_ok findTiffCompression(FILE * fp, int32 * pcomptype);
LEPT_DLL extern l_ok extractG4DataFromFile(const char * filein, uint8 ** pdata, size_t * pnbytes, int32 * pw, int32 * ph, int32 * pminisblack);
LEPT_DLL extern PIX * pixReadMemTiff(const uint8 * cdata, size_t size, int32 n);
LEPT_DLL extern PIX * pixReadMemFromMultipageTiff(const uint8 * cdata, size_t size, size_t * poffset);
LEPT_DLL extern PIXA * pixaReadMemMultipageTiff(const uint8 * data, size_t size);
LEPT_DLL extern l_ok pixaWriteMemMultipageTiff(uint8 ** pdata, size_t * psize, PIXA * pixa);
LEPT_DLL extern l_ok pixWriteMemTiff(uint8 ** pdata, size_t * psize, PIX * pix, int32 comptype);
LEPT_DLL extern l_ok pixWriteMemTiffCustom(uint8 ** pdata, size_t * psize, PIX * pix, int32 comptype, NUMA * natags, SARRAY * savals, SARRAY * satypes, NUMA * nasizes);
LEPT_DLL extern int32 setMsgSeverity(int32 newsev);
LEPT_DLL extern int32 returnErrorInt(const char * msg, const char * procname, int32 ival);
LEPT_DLL extern float returnErrorFloat(const char * msg, const char * procname, float fval);
LEPT_DLL extern void * returnErrorPtr(const char * msg, const char * procname, void * pval);
LEPT_DLL extern void leptSetStderrHandler(void ( * handler )(const char *) );
LEPT_DLL extern void lept_stderr(const char * fmt, ...);
LEPT_DLL extern l_ok filesAreIdentical(const char * fname1, const char * fname2, int32 * psame);
LEPT_DLL extern uint16 convertOnLittleEnd16(uint16 shortin);
LEPT_DLL extern uint16 convertOnBigEnd16(uint16 shortin);
LEPT_DLL extern uint32 convertOnLittleEnd32(uint32 wordin);
LEPT_DLL extern uint32 convertOnBigEnd32(uint32 wordin);
LEPT_DLL extern l_ok fileCorruptByDeletion(const char * filein, float loc, float size, const char * fileout);
LEPT_DLL extern l_ok fileCorruptByMutation(const char * filein, float loc, float size, const char * fileout);
LEPT_DLL extern l_ok fileReplaceBytes(const char * filein, int32 start, int32 nbytes, uint8 * newdata, size_t newsize, const char * fileout);
LEPT_DLL extern l_ok genRandomIntOnInterval(int32 start, int32 end, int32 seed, int32 * pval);
LEPT_DLL extern int32 lept_roundftoi(float fval);
LEPT_DLL extern l_ok l_hashStringToUint64(const char * str, uint64 * phash);
LEPT_DLL extern l_ok l_hashStringToUint64Fast(const char * str, uint64 * phash);
LEPT_DLL extern l_ok l_hashPtToUint64(int32 x, int32 y, uint64 * phash);
LEPT_DLL extern l_ok l_hashFloat64ToUint64(double val, uint64 * phash);
LEPT_DLL extern l_ok findNextLargerPrime(int32 start, uint32 * pprime);
LEPT_DLL extern l_ok lept_isPrime(uint64 n, int32 * pis_prime, uint32 * pfactor);
LEPT_DLL extern uint32 convertIntToGrayCode(uint32 val);
LEPT_DLL extern uint32 convertGrayCodeToInt(uint32 val);
LEPT_DLL extern char * getLeptonicaVersion(void);
LEPT_DLL extern void startTimer(void);
LEPT_DLL extern float stopTimer(void);
LEPT_DLL extern L_TIMER startTimerNested(void);
LEPT_DLL extern float stopTimerNested(L_TIMER rusage_start);
LEPT_DLL extern void l_getCurrentTime(int32 * sec, int32 * usec);
LEPT_DLL extern L_WALLTIMER * startWallTimer(void);
LEPT_DLL extern float stopWallTimer(L_WALLTIMER ** ptimer);
LEPT_DLL extern char * l_getFormattedDate(void);
LEPT_DLL extern char * stringNew(const char * src);
LEPT_DLL extern l_ok stringCopy(char * dest, const char * src, int32 n);
LEPT_DLL extern char * stringCopySegment(const char * src, int32 start, int32 nbytes);
LEPT_DLL extern l_ok stringReplace(char ** pdest, const char * src);
LEPT_DLL extern int32 stringLength(const char * src, size_t size);
LEPT_DLL extern int32 stringCat(char * dest, size_t size, const char * src);
LEPT_DLL extern char * stringConcatNew(const char * first, ...);
LEPT_DLL extern char * stringJoin(const char * src1, const char * src2);
LEPT_DLL extern l_ok stringJoinIP(char ** psrc1, const char * src2);
LEPT_DLL extern char * stringReverse(const char * src);
LEPT_DLL extern char * strtokSafe(char * cstr, const char * seps, char ** psaveptr);
LEPT_DLL extern l_ok stringSplitOnToken(char * cstr, const char * seps, char ** phead, char ** ptail);
LEPT_DLL extern l_ok stringCheckForChars(const char * src, const char * chars, int32 * pfound);
LEPT_DLL extern char * stringRemoveChars(const char * src, const char * remchars);
LEPT_DLL extern char * stringReplaceEachSubstr(const char * src, const char * sub1, const char * sub2, int32 * pcount);
LEPT_DLL extern char * stringReplaceSubstr(const char * src, const char * sub1, const char * sub2, int32 * ploc, int32 * pfound);
LEPT_DLL extern L_DNA * stringFindEachSubstr(const char * src, const char * sub);
LEPT_DLL extern int32 stringFindSubstr(const char * src, const char * sub, int32 * ploc);
LEPT_DLL extern uint8 * arrayReplaceEachSequence(const uint8 * datas, size_t dataslen, const uint8 * seq, size_t seqlen, const uint8 * newseq, size_t newseqlen, size_t * pdatadlen, int32 * pcount);
LEPT_DLL extern L_DNA * arrayFindEachSequence(const uint8 * data, size_t datalen, const uint8 * sequence, size_t seqlen);
LEPT_DLL extern l_ok arrayFindSequence(const uint8 * data, size_t datalen, const uint8 * sequence, size_t seqlen, int32 * poffset, int32 * pfound);
LEPT_DLL extern void * reallocNew(void ** pindata, size_t oldsize, size_t newsize);
LEPT_DLL extern uint8 * l_binaryRead(const char * filename, size_t * pnbytes);
LEPT_DLL extern uint8 * l_binaryReadStream(FILE * fp, size_t * pnbytes);
LEPT_DLL extern uint8 * l_binaryReadSelect(const char * filename, size_t start, size_t nbytes, size_t * pnread);
LEPT_DLL extern uint8 * l_binaryReadSelectStream(FILE * fp, size_t start, size_t nbytes, size_t * pnread);
LEPT_DLL extern l_ok l_binaryWrite(const char * filename, const char * operation, const void * data, size_t nbytes);
LEPT_DLL extern size_t nbytesInFile(const char * filename);
LEPT_DLL extern size_t fnbytesInFile(FILE * fp);
LEPT_DLL extern uint8 * l_binaryCopy(const uint8 * datas, size_t size);
LEPT_DLL extern l_ok l_binaryCompare(const uint8 * data1, size_t size1, const uint8 * data2, size_t size2, int32 * psame);
LEPT_DLL extern l_ok fileCopy(const char * srcfile, const char * newfile);
LEPT_DLL extern l_ok fileConcatenate(const char * srcfile, const char * destfile);
LEPT_DLL extern l_ok fileAppendString(const char * filename, const char * str);
LEPT_DLL extern l_ok fileSplitLinesUniform(const char * filename, int32 n, int32 save_empty, const char * rootpath, const char * ext);
LEPT_DLL extern FILE * fopenReadStream(const char * filename);
LEPT_DLL extern FILE * fopenWriteStream(const char * filename, const char * modestring);
LEPT_DLL extern FILE * fopenReadFromMemory(const uint8 * data, size_t size);
LEPT_DLL extern FILE * fopenWriteWinTempfile(void);
LEPT_DLL extern FILE * lept_fopen(const char * filename, const char * mode);
LEPT_DLL extern l_ok lept_fclose(FILE * fp);
LEPT_DLL extern void * lept_calloc(size_t nmemb, size_t size);
LEPT_DLL extern void lept_free(void * ptr);
LEPT_DLL extern int32 lept_mkdir(const char * subdir);
LEPT_DLL extern int32 lept_rmdir(const char * subdir);
LEPT_DLL extern void lept_direxists(const char * dir, int32 * pexists);
LEPT_DLL extern int32 lept_rm_match(const char * subdir, const char * substr);
LEPT_DLL extern int32 lept_rm(const char * subdir, const char * tail);
LEPT_DLL extern int32 lept_rmfile(const char * filepath);
LEPT_DLL extern int32 lept_mv(const char * srcfile, const char * newdir, const char * newtail, char ** pnewpath);
LEPT_DLL extern int32 lept_cp(const char * srcfile, const char * newdir, const char * newtail, char ** pnewpath);
LEPT_DLL extern void callSystemDebug(const char * cmd);
LEPT_DLL extern l_ok splitPathAtDirectory(const char * pathname, char ** pdir, char ** ptail);
LEPT_DLL extern l_ok splitPathAtExtension(const char * pathname, char ** pbasename, char ** pextension);
LEPT_DLL extern char * pathJoin(const char * dir, const char * fname);
LEPT_DLL extern char * appendSubdirs(const char * basedir, const char * subdirs);
LEPT_DLL extern l_ok convertSepCharsInPath(char * path, int32 type);
LEPT_DLL extern char * genPathname(const char * dir, const char * fname);
LEPT_DLL extern l_ok makeTempDirname(char * result, size_t nbytes, const char * subdir);
LEPT_DLL extern l_ok modifyTrailingSlash(char * path, size_t nbytes, int32 flag);
LEPT_DLL extern char * l_makeTempFilename(void);
LEPT_DLL extern int32 extractNumberFromFilename(const char * fname, int32 numpre, int32 numpost);
LEPT_DLL extern PIX * pixSimpleCaptcha(PIX * pixs, int32 border, int32 nterms, uint32 seed, uint32 color, int32 cmapflag);
LEPT_DLL extern PIX * pixRandomHarmonicWarp(PIX * pixs, float xmag, float ymag, float xfreq, float yfreq, int32 nx, int32 ny, uint32 seed, int32 grayval);
LEPT_DLL extern PIX * pixWarpStereoscopic(PIX * pixs, int32 zbend, int32 zshiftt, int32 zshiftb, int32 ybendt, int32 ybendb, int32 redleft);
LEPT_DLL extern PIX * pixStretchHorizontal(PIX * pixs, int32 dir, int32 type, int32 hmax, int32 operation, int32 incolor);
LEPT_DLL extern PIX * pixStretchHorizontalSampled(PIX * pixs, int32 dir, int32 type, int32 hmax, int32 incolor);
LEPT_DLL extern PIX * pixStretchHorizontalLI(PIX * pixs, int32 dir, int32 type, int32 hmax, int32 incolor);
LEPT_DLL extern PIX * pixQuadraticVShear(PIX * pixs, int32 dir, int32 vmaxt, int32 vmaxb, int32 operation, int32 incolor);
LEPT_DLL extern PIX * pixQuadraticVShearSampled(PIX * pixs, int32 dir, int32 vmaxt, int32 vmaxb, int32 incolor);
LEPT_DLL extern PIX * pixQuadraticVShearLI(PIX * pixs, int32 dir, int32 vmaxt, int32 vmaxb, int32 incolor);
LEPT_DLL extern PIX * pixStereoFromPair(PIX * pix1, PIX * pix2, float rwt, float gwt, float bwt);
LEPT_DLL extern L_WSHED * wshedCreate(PIX * pixs, PIX * pixm, int32 mindepth, int32 debugflag);
LEPT_DLL extern void wshedDestroy(L_WSHED ** pwshed);
LEPT_DLL extern l_ok wshedApply(L_WSHED * wshed);
LEPT_DLL extern l_ok wshedBasins(L_WSHED * wshed, PIXA ** ppixa, NUMA ** pnalevels);
LEPT_DLL extern PIX * wshedRenderFill(L_WSHED * wshed);
LEPT_DLL extern PIX * wshedRenderColors(L_WSHED * wshed);
LEPT_DLL extern l_ok pixaWriteWebPAnim(const char * filename, PIXA * pixa, int32 loopcount, int32 duration, int32 quality, int32 lossless);
LEPT_DLL extern l_ok pixaWriteStreamWebPAnim(FILE * fp, PIXA * pixa, int32 loopcount, int32 duration, int32 quality, int32 lossless);
LEPT_DLL extern l_ok pixaWriteMemWebPAnim(uint8 ** pencdata, size_t * pencsize, PIXA * pixa, int32 loopcount, int32 duration, int32 quality, int32 lossless);
LEPT_DLL extern PIX * pixReadStreamWebP(FILE * fp);
LEPT_DLL extern PIX * pixReadMemWebP(const uint8 * filedata, size_t filesize);
LEPT_DLL extern l_ok readHeaderWebP(const char * filename, int32 * pw, int32 * ph, int32 * pspp);
LEPT_DLL extern l_ok readHeaderMemWebP(const uint8 * data, size_t size, int32 * pw, int32 * ph, int32 * pspp);
LEPT_DLL extern l_ok pixWriteWebP(const char * filename, PIX * pixs, int32 quality, int32 lossless);
LEPT_DLL extern l_ok pixWriteStreamWebP(FILE * fp, PIX * pixs, int32 quality, int32 lossless);
LEPT_DLL extern l_ok pixWriteMemWebP(uint8 ** pencdata, size_t * pencsize, PIX * pixs, int32 quality, int32 lossless);
LEPT_DLL extern int32 l_jpegSetQuality(int32 new_quality);
LEPT_DLL extern void setLeptDebugOK(int32 allow);
LEPT_DLL extern l_ok pixaWriteFiles(const char * rootname, PIXA * pixa, int32 format);
LEPT_DLL extern l_ok pixWriteDebug(const char * fname, PIX * pix, int32 format);
LEPT_DLL extern l_ok pixWrite(const char * fname, PIX * pix, int32 format);
LEPT_DLL extern l_ok pixWriteAutoFormat(const char * filename, PIX * pix);
LEPT_DLL extern l_ok pixWriteStream(FILE * fp, PIX * pix, int32 format);
LEPT_DLL extern l_ok pixWriteImpliedFormat(const char * filename, PIX * pix, int32 quality, int32 progressive);
LEPT_DLL extern int32 pixChooseOutputFormat(PIX * pix);
LEPT_DLL extern int32 getImpliedFileFormat(const char * filename);
LEPT_DLL extern l_ok pixGetAutoFormat(PIX * pix, int32 * pformat);
LEPT_DLL extern const char * getFormatExtension(int32 format);
LEPT_DLL extern l_ok pixWriteMem(uint8 ** pdata, size_t * psize, PIX * pix, int32 format);
LEPT_DLL extern l_ok l_fileDisplay(const char * fname, int32 x, int32 y, float scale);
LEPT_DLL extern l_ok pixDisplay(PIX * pixs, int32 x, int32 y);
LEPT_DLL extern l_ok pixDisplayWithTitle(PIX * pixs, int32 x, int32 y, const char * title, int32 dispflag);
LEPT_DLL extern PIX * pixMakeColorSquare(uint32 color, int32 size, int32 addlabel, int32 location, uint32 textcolor);
LEPT_DLL extern void l_chooseDisplayProg(int32 selection);
LEPT_DLL extern void changeFormatForMissingLib(int32 * pformat);
LEPT_DLL extern l_ok pixDisplayWrite(PIX * pixs, int32 reduction);
LEPT_DLL extern uint8 * zlibCompress(const uint8 * datain, size_t nin, size_t * pnout);
LEPT_DLL extern uint8 * zlibUncompress(const uint8 * datain, size_t nin, size_t * pnout);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* NO_PROTOS */
#endif /* LEPTONICA_ALLHEADERS_H */
