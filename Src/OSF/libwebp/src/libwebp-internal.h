// libwebp-internal.h
//
#ifndef __LIBWEBP_INTERNAL_H
#define __LIBWEBP_INTERNAL_H

#include <slib.h>
#ifdef HAVE_CONFIG_H
	#include <libwebp-config.h>
#endif
#include "src/webp/types.h"
//#include "src/dsp/dsp.h"
//
// Speed-critical functions.
// Author: Skal (pascal.massimino@gmail.com)
//
//#ifdef HAVE_CONFIG_H
//#include "src/webp/config.h"
//#endif
//#include "src/webp/types.h"
#define BPS 32   // this is the common stride for enc/dec
//
// WEBP_RESTRICT
//
// Declares a pointer with the restrict type qualifier if available.
// This allows code to hint to the compiler that only this pointer references a
// particular object or memory region within the scope of the block in which it
// is declared. This may allow for improved optimizations due to the lack of
// pointer aliasing. See also:
// https://en.cppreference.com/w/c/language/restrict
//#if defined(__GNUC__)
	//#define WEBP_RESTRICT __restrict__
//#elif defined(_MSC_VER)
	//#define WEBP_RESTRICT __restrict
//#else
	//#define WEBP_RESTRICT
//#endif
//
// CPU detection
//
#if defined(__GNUC__)
	#define LOCAL_GCC_VERSION ((__GNUC__ << 8) | __GNUC_MINOR__)
	#define LOCAL_GCC_PREREQ(maj, min) (LOCAL_GCC_VERSION >= (((maj) << 8) | (min)))
#else
	#define LOCAL_GCC_VERSION 0
	#define LOCAL_GCC_PREREQ(maj, min) 0
#endif
#if defined(__clang__)
	#define LOCAL_CLANG_VERSION ((__clang_major__ << 8) | __clang_minor__)
	#define LOCAL_CLANG_PREREQ(maj, min) (LOCAL_CLANG_VERSION >= (((maj) << 8) | (min)))
#else
	#define LOCAL_CLANG_VERSION 0
	#define LOCAL_CLANG_PREREQ(maj, min) 0
#endif
// @v11.4.0 #ifndef __has_builtin
	// @v11.4.0 #define __has_builtin(x) 0
// @v11.4.0 #endif
#if !defined(HAVE_CONFIG_H)
#if defined(_MSC_VER) && _MSC_VER > 1310 && (defined(_M_X64) || defined(_M_IX86))
	#define WEBP_MSC_SSE2  // Visual C++ SSE2 targets
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1500 && (defined(_M_X64) || defined(_M_IX86))
	#define WEBP_MSC_SSE41  // Visual C++ SSE4.1 targets
#endif
#endif
// WEBP_HAVE_* are used to indicate the presence of the instruction set in dsp
// files without intrinsics, allowing the corresponding Init() to be called.
// Files containing intrinsics will need to be built targeting the instruction
// set so should succeed on one of the earlier tests.
#if (defined(__SSE2__) || defined(WEBP_MSC_SSE2)) && (!defined(HAVE_CONFIG_H) || defined(WEBP_HAVE_SSE2))
	#define WEBP_USE_SSE2
#endif
#if defined(WEBP_USE_SSE2) && !defined(WEBP_HAVE_SSE2)
	#define WEBP_HAVE_SSE2
#endif
#if (defined(__SSE4_1__) || defined(WEBP_MSC_SSE41)) && (!defined(HAVE_CONFIG_H) || defined(WEBP_HAVE_SSE41))
	#define WEBP_USE_SSE41
#endif
#if defined(WEBP_USE_SSE41) && !defined(WEBP_HAVE_SSE41)
	#define WEBP_HAVE_SSE41
#endif
#undef WEBP_MSC_SSE41
#undef WEBP_MSC_SSE2
// The intrinsics currently cause compiler errors with arm-nacl-gcc and the
// inline assembly would need to be modified for use with Native Client.
#if ((defined(__ARM_NEON__) || defined(__aarch64__)) && (!defined(HAVE_CONFIG_H) || defined(WEBP_HAVE_NEON))) && !defined(__native_client__)
	#define WEBP_USE_NEON
#endif
#if !defined(WEBP_USE_NEON) && defined(__ANDROID__) && defined(__ARM_ARCH_7A__) && defined(HAVE_CPU_FEATURES_H)
	#define WEBP_ANDROID_NEON  // Android targets that may have NEON
	#define WEBP_USE_NEON
#endif
// Note: ARM64 is supported in Visual Studio 2017, but requires the direct
// inclusion of arm64_neon.h; Visual Studio 2019 includes this file in
// arm_neon.h.
#if defined(_MSC_VER) && ((_MSC_VER >= 1700 && defined(_M_ARM)) || (_MSC_VER >= 1920 && defined(_M_ARM64)))
	#define WEBP_USE_NEON
	#define WEBP_USE_INTRINSICS
#endif
#if defined(WEBP_USE_NEON) && !defined(WEBP_HAVE_NEON)
	#define WEBP_HAVE_NEON
#endif
#if defined(__mips__) && !defined(__mips64) && defined(__mips_isa_rev) && (__mips_isa_rev >= 1) && (__mips_isa_rev < 6)
	#define WEBP_USE_MIPS32
	#if (__mips_isa_rev >= 2)
		#define WEBP_USE_MIPS32_R2
		#if defined(__mips_dspr2) || (defined(__mips_dsp_rev) && __mips_dsp_rev >= 2)
			#define WEBP_USE_MIPS_DSP_R2
		#endif
	#endif
#endif
#if defined(__mips_msa) && defined(__mips_isa_rev) && (__mips_isa_rev >= 5)
	#define WEBP_USE_MSA
#endif
#ifndef WEBP_DSP_OMIT_C_CODE
	#define WEBP_DSP_OMIT_C_CODE 1
#endif
#if defined(WEBP_USE_NEON) && WEBP_DSP_OMIT_C_CODE
	#define WEBP_NEON_OMIT_C_CODE 1
#else
	#define WEBP_NEON_OMIT_C_CODE 0
#endif
#if !(LOCAL_CLANG_PREREQ(3, 8) || LOCAL_GCC_PREREQ(4, 8) || defined(__aarch64__))
	#define WEBP_NEON_WORK_AROUND_GCC 1
#else
	#define WEBP_NEON_WORK_AROUND_GCC 0
#endif
// This macro prevents thread_sanitizer from reporting known concurrent writes.
#define WEBP_TSAN_IGNORE_FUNCTION
#if defined(__has_feature)
	#if __has_feature(thread_sanitizer)
		#undef WEBP_TSAN_IGNORE_FUNCTION
		#define WEBP_TSAN_IGNORE_FUNCTION __attribute__((no_sanitize_thread))
	#endif
#endif
#if defined(WEBP_USE_THREAD) && !defined(_WIN32)
#include <pthread.h>  // NOLINT

#define WEBP_DSP_INIT(func) do {                                    \
		static volatile VP8CPUInfo func ## _last_cpuinfo_used = (VP8CPUInfo)&func ## _last_cpuinfo_used; \
		static pthread_mutex_t func ## _lock = PTHREAD_MUTEX_INITIALIZER; \
		if(pthread_mutex_lock(&func ## _lock)) break;                    \
		if(func ## _last_cpuinfo_used != VP8GetCPUInfo) func();          \
		func ## _last_cpuinfo_used = VP8GetCPUInfo;                       \
		(void)pthread_mutex_unlock(&func ## _lock);                       \
} while(0)
#else  // !(defined(WEBP_USE_THREAD) && !defined(_WIN32))
#define WEBP_DSP_INIT(func) do {                                    \
		static volatile VP8CPUInfo func ## _last_cpuinfo_used =(VP8CPUInfo)&func ## _last_cpuinfo_used; \
		if(func ## _last_cpuinfo_used == VP8GetCPUInfo) break;           \
		func();                                                           \
		func ## _last_cpuinfo_used = VP8GetCPUInfo;                       \
} while(0)
#endif  // defined(WEBP_USE_THREAD) && !defined(_WIN32)

// Defines an Init + helper function that control multiple initialization of
// function pointers / tables.
/* Usage:
   WEBP_DSP_INIT_FUNC(InitFunc) {
     ...function body
   }
 */
#define WEBP_DSP_INIT_FUNC(name)                             \
	static WEBP_TSAN_IGNORE_FUNCTION void name ## _body(void); \
	WEBP_TSAN_IGNORE_FUNCTION void name(void) {                \
		WEBP_DSP_INIT(name ## _body);                            \
	}                                                          \
	static WEBP_TSAN_IGNORE_FUNCTION void name ## _body(void)

#define WEBP_UBSAN_IGNORE_UNDEF
#define WEBP_UBSAN_IGNORE_UNSIGNED_OVERFLOW
#if defined(__clang__) && defined(__has_attribute)
#if __has_attribute(no_sanitize)
// This macro prevents the undefined behavior sanitizer from reporting
// failures. This is only meant to silence unaligned loads on platforms that
// are known to support them.
#undef WEBP_UBSAN_IGNORE_UNDEF
#define WEBP_UBSAN_IGNORE_UNDEF __attribute__((no_sanitize("undefined")))
// This macro prevents the undefined behavior sanitizer from reporting
// failures related to unsigned integer overflows. This is only meant to
// silence cases where this well defined behavior is expected.
#undef WEBP_UBSAN_IGNORE_UNSIGNED_OVERFLOW
	#define WEBP_UBSAN_IGNORE_UNSIGNED_OVERFLOW __attribute__((no_sanitize("unsigned-integer-overflow")))
#endif
#endif
// If 'ptr' is NULL, returns NULL. Otherwise returns 'ptr + off'.
// Prevents undefined behavior sanitizer nullptr-with-nonzero-offset warning.
#if !defined(WEBP_OFFSET_PTR)
	#define WEBP_OFFSET_PTR(ptr, off) (((ptr) == NULL) ? NULL : ((ptr) + (off)))
#endif
// Regularize the definition of WEBP_SWAP_16BIT_CSP (backward compatibility)
#if !defined(WEBP_SWAP_16BIT_CSP)
	#define WEBP_SWAP_16BIT_CSP 0
#endif
// some endian fix (e.g.: mips-gcc doesn't define __BIG_ENDIAN__)
//#if !defined(WORDS_BIGENDIAN) && (defined(__BIG_ENDIAN__) || defined(_M_PPC) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)))
	//#define WORDS_BIGENDIAN
//#endif

typedef enum {
	kSSE2,
	kSSE3,
	kSlowSSSE3, // special feature for slow SSSE3 architectures
	kSSE4_1,
	kAVX,
	kAVX2,
	kNEON,
	kMIPS32,
	kMIPSdspR2,
	kMSA
} CPUFeature;
// returns true if the CPU supports the feature.
typedef int (* VP8CPUInfo)(CPUFeature feature);
/*WEBP_EXTERN*/extern VP8CPUInfo VP8GetCPUInfo;
//
// Init stub generator
//
// Defines an init function stub to ensure each module exposes a symbol,
// avoiding a compiler warning.
#define WEBP_DSP_INIT_STUB(func) extern void func(void); void func(void) {}
//
// Encoding
//
// Transforms
// VP8Idct: Does one of two inverse transforms. If do_two is set, the transforms
//          will be done for (ref, in, dst) and (ref + 4, in + 16, dst + 4).
typedef void (* VP8Idct)(const uint8* ref, const int16_t* in, uint8* dst, int do_two);
typedef void (* VP8Fdct)(const uint8* src, const uint8* ref, int16_t* out);
typedef void (* VP8WHT)(const int16_t* in, int16_t* out);
extern VP8Idct VP8ITransform;
extern VP8Fdct VP8FTransform;
extern VP8Fdct VP8FTransform2;   // performs two transforms at a time
extern VP8WHT VP8FTransformWHT;
// Predictions
// *dst is the destination block. *top and *left can be NULL.
typedef void (* VP8IntraPreds)(uint8* dst, const uint8* left, const uint8* top);
typedef void (* VP8Intra4Preds)(uint8* dst, const uint8* top);
extern VP8Intra4Preds VP8EncPredLuma4;
extern VP8IntraPreds VP8EncPredLuma16;
extern VP8IntraPreds VP8EncPredChroma8;

typedef int (* VP8Metric)(const uint8* pix, const uint8* ref);
extern VP8Metric VP8SSE16x16, VP8SSE16x8, VP8SSE8x8, VP8SSE4x4;
typedef int (* VP8WMetric)(const uint8* pix, const uint8* ref, const uint16_t* const weights);
// The weights for VP8TDisto4x4 and VP8TDisto16x16 contain a row-major
// 4 by 4 symmetric matrix.
extern VP8WMetric VP8TDisto4x4, VP8TDisto16x16;

// Compute the average (DC) of four 4x4 blocks.
// Each sub-4x4 block #i sum is stored in dc[i].
typedef void (* VP8MeanMetric)(const uint8* ref, uint32_t dc[4]);
extern VP8MeanMetric VP8Mean16x4;

typedef void (* VP8BlockCopy)(const uint8* src, uint8* dst);
extern VP8BlockCopy VP8Copy4x4;
extern VP8BlockCopy VP8Copy16x8;
// Quantization
struct VP8Matrix;   // forward declaration

typedef int (* VP8QuantizeBlock)(int16_t in[16], int16_t out[16], const struct VP8Matrix* const mtx);
// Same as VP8QuantizeBlock, but quantizes two consecutive blocks.
typedef int (* VP8Quantize2Blocks)(int16_t in[32], int16_t out[32], const struct VP8Matrix* const mtx);
extern VP8QuantizeBlock VP8EncQuantizeBlock;
extern VP8Quantize2Blocks VP8EncQuantize2Blocks;

// specific to 2nd transform:
typedef int (* VP8QuantizeBlockWHT)(int16_t in[16], int16_t out[16], const struct VP8Matrix* const mtx);
extern VP8QuantizeBlockWHT VP8EncQuantizeBlockWHT;

extern const int VP8DspScan[16 + 4 + 4];

// Collect histogram for susceptibility calculation.
#define MAX_COEFF_THRESH   31   // size of histogram used by CollectHistogram.
typedef struct {
	// We only need to store max_value and last_non_zero, not the distribution.
	int max_value;
	int last_non_zero;
} VP8Histogram;
typedef void (* VP8CHisto)(const uint8* ref, const uint8* pred, int start_block, int end_block, VP8Histogram* const histo);
extern VP8CHisto VP8CollectHistogram;
// General-purpose util function to help VP8CollectHistogram().
void VP8SetHistogramData(const int distribution[MAX_COEFF_THRESH + 1], VP8Histogram* const histo);

// must be called before using any of the above
void VP8EncDspInit(void);

//------------------------------------------------------------------------------
// cost functions (encoding)

extern const uint16_t VP8EntropyCost[256];        // 8bit fixed-point log(p)
// approximate cost per level:
extern const uint16_t VP8LevelFixedCosts[2047 /*MAX_LEVEL*/ + 1];
extern const uint8 VP8EncBands[16 + 1];

struct VP8Residual;

typedef void (* VP8SetResidualCoeffsFunc)(const int16_t* const coeffs, struct VP8Residual* const res);
extern VP8SetResidualCoeffsFunc VP8SetResidualCoeffs;

// Cost calculation function.
typedef int (* VP8GetResidualCostFunc)(int ctx0, const struct VP8Residual* const res);
extern VP8GetResidualCostFunc VP8GetResidualCost;

// must be called before anything using the above
void VP8EncDspCostInit(void);

//------------------------------------------------------------------------------
// SSIM / PSNR utils

// struct for accumulating statistical moments
typedef struct {
	uint32_t w;        // sum(w_i) : sum of weights
	uint32_t xm, ym;   // sum(w_i * x_i), sum(w_i * y_i)
	uint32_t xxm, xym, yym; // sum(w_i * x_i * x_i), etc.
} VP8DistoStats;

// Compute the final SSIM value
// The non-clipped version assumes stats->w = (2 * VP8_SSIM_KERNEL + 1)^2.
double VP8SSIMFromStats(const VP8DistoStats* const stats);
double VP8SSIMFromStatsClipped(const VP8DistoStats* const stats);

#define VP8_SSIM_KERNEL 3   // total size of the kernel: 2 * VP8_SSIM_KERNEL + 1
typedef double (* VP8SSIMGetClippedFunc)(const uint8* src1, int stride1, const uint8* src2, int stride2, int xo, int yo/*center position*/, int W, int H/*plane dimension*/);
#if !defined(WEBP_REDUCE_SIZE)
	// This version is called with the guarantee that you can load 8 bytes and 8 rows at offset src1 and src2
	typedef double (* VP8SSIMGetFunc)(const uint8* src1, int stride1, const uint8* src2, int stride2);
	extern VP8SSIMGetFunc VP8SSIMGet;         // unclipped / unchecked
	extern VP8SSIMGetClippedFunc VP8SSIMGetClipped;   // with clipping
#endif
#if !defined(WEBP_DISABLE_STATS)
	typedef uint32_t (* VP8AccumulateSSEFunc)(const uint8* src1, const uint8* src2, int len);
	extern VP8AccumulateSSEFunc VP8AccumulateSSE;
#endif
// must be called before using any of the above directly
void VP8SSIMDspInit(void);
//
// Decoding
//
typedef void (* VP8DecIdct)(const int16_t* coeffs, uint8* dst);
// when doing two transforms, coeffs is actually int16_t[2][16].
typedef void (* VP8DecIdct2)(const int16_t* coeffs, uint8* dst, int do_two);
extern VP8DecIdct2 VP8Transform;
extern VP8DecIdct VP8TransformAC3;
extern VP8DecIdct VP8TransformUV;
extern VP8DecIdct VP8TransformDC;
extern VP8DecIdct VP8TransformDCUV;
extern VP8WHT VP8TransformWHT;

// *dst is the destination block, with stride BPS. Boundary samples are
// assumed accessible when needed.
typedef void (* VP8PredFunc)(uint8* dst);
extern VP8PredFunc VP8PredLuma16[] /* NUM_B_DC_MODES */;
extern VP8PredFunc VP8PredChroma8[] /* NUM_B_DC_MODES */;
extern VP8PredFunc VP8PredLuma4[] /* NUM_BMODES */;

// clipping tables (for filtering)
extern const int8_t* const VP8ksclip1;  // clips [-1020, 1020] to [-128, 127]
extern const int8_t* const VP8ksclip2;  // clips [-112, 112] to [-16, 15]
extern const uint8* const VP8kclip1;  // clips [-255,511] to [0,255]
extern const uint8* const VP8kabs0;   // abs(x) for x in [-255,255]
// must be called first
void VP8InitClipTables(void);

// simple filter (only for luma)
typedef void (* VP8SimpleFilterFunc)(uint8* p, int stride, int thresh);
extern VP8SimpleFilterFunc VP8SimpleVFilter16;
extern VP8SimpleFilterFunc VP8SimpleHFilter16;
extern VP8SimpleFilterFunc VP8SimpleVFilter16i;  // filter 3 inner edges
extern VP8SimpleFilterFunc VP8SimpleHFilter16i;

// regular filter (on both macroblock edges and inner edges)
typedef void (* VP8LumaFilterFunc)(uint8* luma, int stride, int thresh, int ithresh, int hev_t);
typedef void (* VP8ChromaFilterFunc)(uint8* u, uint8* v, int stride, int thresh, int ithresh, int hev_t);
// on outer edge
extern VP8LumaFilterFunc VP8VFilter16;
extern VP8LumaFilterFunc VP8HFilter16;
extern VP8ChromaFilterFunc VP8VFilter8;
extern VP8ChromaFilterFunc VP8HFilter8;

// on inner edge
extern VP8LumaFilterFunc VP8VFilter16i;   // filtering 3 inner edges altogether
extern VP8LumaFilterFunc VP8HFilter16i;
extern VP8ChromaFilterFunc VP8VFilter8i;  // filtering u and v altogether
extern VP8ChromaFilterFunc VP8HFilter8i;

// Dithering. Combines dithering values (centered around 128) with dst[],
// according to: dst[] = clip(dst[] + (((dither[]-128) + 8) >> 4)
#define VP8_DITHER_DESCALE 4
#define VP8_DITHER_DESCALE_ROUNDER (1 << (VP8_DITHER_DESCALE - 1))
#define VP8_DITHER_AMP_BITS 7
#define VP8_DITHER_AMP_CENTER (1 << VP8_DITHER_AMP_BITS)
extern void (* VP8DitherCombine8x8)(const uint8* dither, uint8* dst, int dst_stride);
// must be called before anything using the above
void VP8DspInit(void);
//
// WebP I/O
//
#define FANCY_UPSAMPLING   // undefined to remove fancy upsampling support

// Convert a pair of y/u/v lines together to the output rgb/a colorspace.
// bottom_y can be NULL if only one line of output is needed (at top/bottom).
typedef void (* WebPUpsampleLinePairFunc)(const uint8* top_y, const uint8* bottom_y, const uint8* top_u, const uint8* top_v,
    const uint8* cur_u, const uint8* cur_v, uint8* top_dst, uint8* bottom_dst, int len);
#ifdef FANCY_UPSAMPLING
	// Fancy upsampling functions to convert YUV to RGB(A) modes
	extern WebPUpsampleLinePairFunc WebPUpsamplers[] /* MODE_LAST */;
#endif    // FANCY_UPSAMPLING

// Per-row point-sampling methods.
typedef void (* WebPSamplerRowFunc)(const uint8* y, const uint8* u, const uint8* v, uint8* dst, int len);
// Generic function to apply 'WebPSamplerRowFunc' to the whole plane:
void WebPSamplerProcessPlane(const uint8* y, int y_stride, const uint8* u, const uint8* v, int uv_stride, uint8* dst, int dst_stride, int width, int height, WebPSamplerRowFunc func);
// Sampling functions to convert rows of YUV to RGB(A)
extern WebPSamplerRowFunc WebPSamplers[] /* MODE_LAST */;
// General function for converting two lines of ARGB or RGBA.
// 'alpha_is_last' should be true if 0xff000000 is stored in memory as
// as 0x00, 0x00, 0x00, 0xff (little endian).
WebPUpsampleLinePairFunc WebPGetLinePairConverter(int alpha_is_last);
// YUV444->RGB converters
typedef void (* WebPYUV444Converter)(const uint8* y, const uint8* u, const uint8* v, uint8* dst, int len);
extern WebPYUV444Converter WebPYUV444Converters[] /* MODE_LAST */;
// Must be called before using the WebPUpsamplers[] (and for premultiplied
// colorspaces like rgbA, rgbA4444, etc)
void WebPInitUpsamplers(void);
// Must be called before using WebPSamplers[]
void WebPInitSamplers(void);
// Must be called before using WebPYUV444Converters[]
void WebPInitYUV444Converters(void);
//
// ARGB -> YUV converters
//
// Convert ARGB samples to luma Y.
extern void (* WebPConvertARGBToY)(const uint32_t* argb, uint8* y, int width);
// Convert ARGB samples to U/V with downsampling. do_store should be '1' for
// even lines and '0' for odd ones. 'src_width' is the original width, not
// the U/V one.
extern void (* WebPConvertARGBToUV)(const uint32_t* argb, uint8* u, uint8* v, int src_width, int do_store);
// Convert a row of accumulated (four-values) of rgba32 toward U/V
extern void (* WebPConvertRGBA32ToUV)(const uint16_t* rgb, uint8* u, uint8* v, int width);
// Convert RGB or BGR to Y
extern void (* WebPConvertRGB24ToY)(const uint8* rgb, uint8* y, int width);
extern void (* WebPConvertBGR24ToY)(const uint8* bgr, uint8* y, int width);
// used for plain-C fallback.
extern void WebPConvertARGBToUV_C(const uint32_t* argb, uint8* u, uint8* v, int src_width, int do_store);
extern void WebPConvertRGBA32ToUV_C(const uint16_t* rgb, uint8* u, uint8* v, int width);
// utilities for accurate RGB->YUV conversion
extern uint64_t (* WebPSharpYUVUpdateY)(const uint16_t* src, const uint16_t* ref, uint16_t* dst, int len);
extern void (* WebPSharpYUVUpdateRGB)(const int16_t* src, const int16_t* ref, int16_t* dst, int len);
extern void (* WebPSharpYUVFilterRow)(const int16_t* A, const int16_t* B, int len, const uint16_t* best_y, uint16_t* out);
// Must be called before using the above.
void WebPInitConvertARGBToYUV(void);
//
// Rescaler
//
struct WebPRescaler;

// Import a row of data and save its contribution in the rescaler.
// 'channel' denotes the channel number to be imported. 'Expand' corresponds to
// the wrk->x_expand case. Otherwise, 'Shrink' is to be used.
typedef void (* WebPRescalerImportRowFunc)(struct WebPRescaler* const wrk, const uint8* src);
extern WebPRescalerImportRowFunc WebPRescalerImportRowExpand;
extern WebPRescalerImportRowFunc WebPRescalerImportRowShrink;
// Export one row (starting at x_out position) from rescaler.
// 'Expand' corresponds to the wrk->y_expand case.
// Otherwise 'Shrink' is to be used
typedef void (* WebPRescalerExportRowFunc)(struct WebPRescaler* const wrk);
extern WebPRescalerExportRowFunc WebPRescalerExportRowExpand;
extern WebPRescalerExportRowFunc WebPRescalerExportRowShrink;
// Plain-C implementation, as fall-back.
extern void WebPRescalerImportRowExpand_C(struct WebPRescaler* const wrk, const uint8* src);
extern void WebPRescalerImportRowShrink_C(struct WebPRescaler* const wrk, const uint8* src);
extern void WebPRescalerExportRowExpand_C(struct WebPRescaler* const wrk);
extern void WebPRescalerExportRowShrink_C(struct WebPRescaler* const wrk);
// Main entry calls:
extern void WebPRescalerImportRow(struct WebPRescaler* const wrk, const uint8* src);
// Export one row (starting at x_out position) from rescaler.
extern void WebPRescalerExportRow(struct WebPRescaler* const wrk);
// Must be called first before using the above.
void WebPRescalerDspInit(void);
//
// Utilities for processing transparent channel.
//
// Apply alpha pre-multiply on an rgba, bgra or argb plane of size w * h.
// alpha_first should be 0 for argb, 1 for rgba or bgra (where alpha is last).
extern void (* WebPApplyAlphaMultiply)(uint8* rgba, int alpha_first, int w, int h, int stride);
// Same, buf specifically for RGBA4444 format
extern void (* WebPApplyAlphaMultiply4444)(uint8* rgba4444, int w, int h, int stride);
// Dispatch the values from alpha[] plane to the ARGB destination 'dst'.
// Returns true if alpha[] plane has non-trivial values different from 0xff.
extern int (* WebPDispatchAlpha)(const uint8* _RESTRICT alpha, int alpha_stride, int width, int height, uint8* _RESTRICT dst, int dst_stride);
// Transfer packed 8b alpha[] values to green channel in dst[], zero'ing the
// A/R/B values. 'dst_stride' is the stride for dst[] in uint32_t units.
extern void (* WebPDispatchAlphaToGreen)(const uint8* _RESTRICT alpha, int alpha_stride, int width, int height, uint32_t* _RESTRICT dst, int dst_stride);
// Extract the alpha values from 32b values in argb[] and pack them into alpha[]
// (this is the opposite of WebPDispatchAlpha).
// Returns true if there's only trivial 0xff alpha values.
extern int (* WebPExtractAlpha)(const uint8* _RESTRICT argb, int argb_stride, int width, int height, uint8* _RESTRICT alpha, int alpha_stride);

// Extract the green values from 32b values in argb[] and pack them into alpha[]
// (this is the opposite of WebPDispatchAlphaToGreen).
extern void (* WebPExtractGreen)(const uint32_t* _RESTRICT argb, uint8* _RESTRICT alpha, int size);

// Pre-Multiply operation transforms x into x * A / 255  (where x=Y,R,G or B).
// Un-Multiply operation transforms x into x * 255 / A.

// Pre-Multiply or Un-Multiply (if 'inverse' is true) argb values in a row.
extern void (* WebPMultARGBRow)(uint32_t* const ptr, int width, int inverse);
// Same a WebPMultARGBRow(), but for several rows.
void WebPMultARGBRows(uint8* ptr, int stride, int width, int num_rows, int inverse);
// Same for a row of single values, with side alpha values.
extern void (* WebPMultRow)(uint8* _RESTRICT const ptr, const uint8* _RESTRICT const alpha, int width, int inverse);
// Same a WebPMultRow(), but for several 'num_rows' rows.
void WebPMultRows(uint8* _RESTRICT ptr, int stride, const uint8* _RESTRICT alpha, int alpha_stride, int width, int num_rows, int inverse);
// Plain-C versions, used as fallback by some implementations.
void WebPMultRow_C(uint8* _RESTRICT const ptr, const uint8* _RESTRICT const alpha, int width, int inverse);
void WebPMultARGBRow_C(uint32_t* const ptr, int width, int inverse);
#ifdef SL_BIGENDIAN
	// ARGB packing function: a/r/g/b input is rgba or bgra order.
	extern void (* WebPPackARGB)(const uint8* _RESTRICT a, const uint8* _RESTRICT r, const uint8* _RESTRICT g, const uint8* _RESTRICT b, int len, uint32_t* _RESTRICT out);
#endif
// RGB packing function. 'step' can be 3 or 4. r/g/b input is rgb or bgr order.
extern void (* WebPPackRGB)(const uint8* _RESTRICT r, const uint8* _RESTRICT g, const uint8* _RESTRICT b, int len, int step, uint32_t* _RESTRICT out);
// This function returns true if src[i] contains a value different from 0xff.
extern int (* WebPHasAlpha8b)(const uint8* src, int length);
// This function returns true if src[4*i] contains a value different from 0xff.
extern int (* WebPHasAlpha32b)(const uint8* src, int length);
// replaces transparent values in src[] by 'color'.
extern void (* WebPAlphaReplace)(uint32_t* src, int length, uint32_t color);

// To be called first before using the above.
void WebPInitAlphaProcessing(void);
//
// Filter functions
//
typedef enum {     // Filter types.
	WEBP_FILTER_NONE = 0,
	WEBP_FILTER_HORIZONTAL,
	WEBP_FILTER_VERTICAL,
	WEBP_FILTER_GRADIENT,
	WEBP_FILTER_LAST = WEBP_FILTER_GRADIENT + 1, // end marker
	WEBP_FILTER_BEST, // meta-types
	WEBP_FILTER_FAST
} WEBP_FILTER_TYPE;

typedef void (* WebPFilterFunc)(const uint8* in, int width, int height, int stride, uint8* out);
// In-place un-filtering.
// Warning! 'prev_line' pointer can be equal to 'cur_line' or 'preds'.
typedef void (* WebPUnfilterFunc)(const uint8* prev_line, const uint8* preds, uint8* cur_line, int width);

// Filter the given data using the given predictor.
// 'in' corresponds to a 2-dimensional pixel array of size (stride * height)
// in raster order.
// 'stride' is number of bytes per scan line (with possible padding).
// 'out' should be pre-allocated.
extern WebPFilterFunc WebPFilters[WEBP_FILTER_LAST];

// In-place reconstruct the original data from the given filtered data.
// The reconstruction will be done for 'num_rows' rows starting from 'row'
// (assuming rows upto 'row - 1' are already reconstructed).
extern WebPUnfilterFunc WebPUnfilters[WEBP_FILTER_LAST];

// To be called first before using the above.
void VP8FiltersInit(void);
//
#include "src/utils/rescaler_utils.h"
#include "src/webp/decode.h"
#include "src/dec/vp8_dec.h"
#include "src/dec/webpi_dec.h"
#include "src/utils/filters_utils.h"
#include "src/dec/alphai_dec.h"
#include "src/utils/utils.h"
#include "src/dec/vp8i_dec.h"
#include "src/dec/vp8li_dec.h"
#include "src/utils/bit_reader_utils.h"
//#include "src/utils/endian_inl_utils.h"
//
// Endian related functions.
//
//#if defined(SL_BIGENDIAN)
	//#define HToLE32 BSwap32
	//#define HToLE16 BSwap16
//#else
	//#define HToLE32(x) (x)
	//#define HToLE16(x) (x)
//#endif
#if !defined(HAVE_CONFIG_H)
	#if LOCAL_GCC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
		#define HAVE_BUILTIN_BSWAP16
	#endif
	#if LOCAL_GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
		#define HAVE_BUILTIN_BSWAP32
	#endif
	#if LOCAL_GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
		#define HAVE_BUILTIN_BSWAP64
	#endif
#endif  // !HAVE_CONFIG_H
/* @sobolev
static FORCEINLINE uint16_t BSwap16(uint16_t x) 
{
#if defined(HAVE_BUILTIN_BSWAP16)
	return __builtin_bswap16(x);
#elif defined(_MSC_VER)
	return _byteswap_ushort(x);
#else
	// gcc will recognize a 'rorw $8, ...' here:
	return (x >> 8) | ((x & 0xff) << 8);
#endif  // HAVE_BUILTIN_BSWAP16
}

static FORCEINLINE uint32_t BSwap32(uint32_t x) 
{
#if defined(WEBP_USE_MIPS32_R2)
	uint32_t ret;
	__asm__ volatile (
		"wsbh   %[ret], %[x]          \n\t"
		"rotr   %[ret], %[ret],  16   \n\t"
		: [ret] "=r" (ret)
		: [x] "r" (x)
		);
	return ret;
#elif defined(HAVE_BUILTIN_BSWAP32)
	return __builtin_bswap32(x);
#elif defined(__i386__) || defined(__x86_64__)
	uint32_t swapped_bytes;
	__asm__ volatile ("bswap %0" : "=r" (swapped_bytes) : "0" (x));
	return swapped_bytes;
#elif defined(_MSC_VER)
	return (uint32_t)_byteswap_ulong(x);
#else
	return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
#endif  // HAVE_BUILTIN_BSWAP32
}

static FORCEINLINE uint64_t BSwap64(uint64_t x) 
{
#if defined(HAVE_BUILTIN_BSWAP64)
	return __builtin_bswap64(x);
#elif defined(__x86_64__)
	uint64_t swapped_bytes;
	__asm__ volatile ("bswapq %0" : "=r" (swapped_bytes) : "0" (x));
	return swapped_bytes;
#elif defined(_MSC_VER)
	return (uint64_t)_byteswap_uint64(x);
#else  // generic code for swapping 64-bit values (suggested by bdb@)
	x = ((x & 0xffffffff00000000ull) >> 32) | ((x & 0x00000000ffffffffull) << 32);
	x = ((x & 0xffff0000ffff0000ull) >> 16) | ((x & 0x0000ffff0000ffffull) << 16);
	x = ((x & 0xff00ff00ff00ff00ull) >>  8) | ((x & 0x00ff00ff00ff00ffull) <<  8);
	return x;
#endif  // HAVE_BUILTIN_BSWAP64
}*/
//
//#include "src/utils/bit_reader_inl_utils.h"
//
// Specific inlined methods for boolean decoder [VP8GetBit() ...]
// This file should be included by the .c sources that actually need to call
// these methods.
//
// Author: Skal (pascal.massimino@gmail.com)
//
// Derived type lbit_t = natural type for memory I/O
//
#if (BITS > 32)
	typedef uint64_t lbit_t;
#elif (BITS > 16)
	typedef uint32_t lbit_t;
#elif (BITS >  8)
	typedef uint16_t lbit_t;
#else
	typedef uint8 lbit_t;
#endif

extern const uint8 kVP8Log2Range[128];
extern const uint8 kVP8NewRange[128];

// special case for the tail byte-reading
void VP8LoadFinalBytes(VP8BitReader* const br);
//
// Inlined critical functions
//
// makes sure br->value_ has at least BITS bits worth of data
static WEBP_UBSAN_IGNORE_UNDEF FORCEINLINE void VP8LoadNewBytes(VP8BitReader* _RESTRICT const br) 
{
	assert(br != NULL && br->buf_ != NULL);
	// Read 'BITS' bits at a time if possible.
	if(br->buf_ < br->buf_max_) {
		// convert memory type to register type (with some zero'ing!)
		bit_t bits;
#if defined(WEBP_USE_MIPS32)
		// This is needed because of un-aligned read.
		lbit_t in_bits;
		lbit_t* p_buf_ = (lbit_t*)br->buf_;
		__asm__ volatile (
			".set   push                             \n\t"
			".set   at                               \n\t"
			".set   macro                            \n\t"
			"ulw    %[in_bits], 0(%[p_buf_])         \n\t"
			".set   pop                              \n\t"
			: [in_bits] "=r" (in_bits)
			: [p_buf_] "r" (p_buf_)
			: "memory", "at"
			);
#else
		lbit_t in_bits;
		memcpy(&in_bits, br->buf_, sizeof(in_bits));
#endif
		br->buf_ += BITS >> 3;
#if !defined(SL_BIGENDIAN)
	#if (BITS > 32)
			bits = /*BSwap64*/SMem::BSwap(in_bits);
			bits >>= 64 - BITS;
	#elif (BITS >= 24)
			bits = /*BSwap32*/SMem::BSwap(in_bits);
			bits >>= (32 - BITS);
	#elif (BITS == 16)
			bits = /*BSwap16*/SMem::BSwap(in_bits);
	#else   // BITS == 8
			bits = (bit_t)in_bits;
	#endif  // BITS > 32
#else
		bits = (bit_t)in_bits;
		if(BITS != 8 * sizeof(bit_t)) 
			bits >>= (8 * sizeof(bit_t) - BITS);
#endif
		br->value_ = bits | (br->value_ << BITS);
		br->bits_ += BITS;
	}
	else {
		VP8LoadFinalBytes(br); // no need to be inlined
	}
}

// Read a bit with proba 'prob'. Speed-critical function!
static FORCEINLINE int VP8GetBit(VP8BitReader* _RESTRICT const br, int prob, const char label[]) 
{
	// Don't move this declaration! It makes a big speed difference to store
	// 'range' *before* calling VP8LoadNewBytes(), even if this function doesn't
	// alter br->range_ value.
	range_t range = br->range_;
	if(br->bits_ < 0) {
		VP8LoadNewBytes(br);
	}
	{
		const int pos = br->bits_;
		const range_t split = (range * prob) >> 8;
		const range_t value = (range_t)(br->value_ >> pos);
		const int bit = (value > split);
		if(bit) {
			range -= split;
			br->value_ -= (bit_t)(split + 1) << pos;
		}
		else {
			range = split + 1;
		}
		{
			const int shift = 7 ^ BitsLog2Floor(range);
			range <<= shift;
			br->bits_ -= shift;
		}
		br->range_ = range - 1;
		BT_TRACK(br);
		return bit;
	}
}

// simplified version of VP8GetBit() for prob=0x80 (note shift is always 1 here)
static WEBP_UBSAN_IGNORE_UNSIGNED_OVERFLOW FORCEINLINE int VP8GetSigned(VP8BitReader* _RESTRICT const br, int v, const char label[]) 
{
	if(br->bits_ < 0) {
		VP8LoadNewBytes(br);
	}
	{
		const int pos = br->bits_;
		const range_t split = br->range_ >> 1;
		const range_t value = (range_t)(br->value_ >> pos);
		const int32_t mask = (int32_t)(split - value) >> 31; // -1 or 0
		br->bits_ -= 1;
		br->range_ += mask;
		br->range_ |= 1;
		br->value_ -= (bit_t)((split + 1) & mask) << pos;
		BT_TRACK(br);
		return (v ^ mask) - mask;
	}
}

static FORCEINLINE int VP8GetBitAlt(VP8BitReader* _RESTRICT const br, int prob, const char label[]) 
{
	// Don't move this declaration! It makes a big speed difference to store
	// 'range' *before* calling VP8LoadNewBytes(), even if this function doesn't
	// alter br->range_ value.
	range_t range = br->range_;
	if(br->bits_ < 0) {
		VP8LoadNewBytes(br);
	}
	{
		const int pos = br->bits_;
		const range_t split = (range * prob) >> 8;
		const range_t value = (range_t)(br->value_ >> pos);
		int bit; // Don't use 'const int bit = (value > split);", it's slower.
		if(value > split) {
			range -= split + 1;
			br->value_ -= (bit_t)(split + 1) << pos;
			bit = 1;
		}
		else {
			range = split;
			bit = 0;
		}
		if(range <= (range_t)0x7e) {
			const int shift = kVP8Log2Range[range];
			range = kVP8NewRange[range];
			br->bits_ -= shift;
		}
		br->range_ = range;
		BT_TRACK(br);
		return bit;
	}
}
//
#include "src/webp/mux.h"
#include "src/mux/muxi.h"

#endif // !__LIBWEBP_INTERNAL_H

