// ZXING-INTERNAL.H
//
#ifndef __ZXING_INTERNAL_H
#define __ZXING_INTERNAL_H

#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#include <cstdio>
#include <string_view>
#include <initializer_list>

//#include "ZXConfig.h"
// Thread local or static memory may be used to reduce the number of (re-)allocations of temporary variables
// in e.g. the ReedSolomonDecoder. It is disabled by default. It can be enabled by modifying the following define.
// Note: The Apple clang compiler until XCode 8 does not support c++11's thread_local.
// The alternative 'static' makes the code thread unsafe.
#define ZX_THREAD_LOCAL // '' (nothing), 'thread_local' or 'static'

// The Galoir Field abstractions used in Reed-Solomon error correction code can use more memory to eliminate a modulo
// operation. This improves performance but might not be the best option if RAM is scarce. The effect is a few kB big.
#define ZX_REED_SOLOMON_USE_MORE_MEMORY_FOR_SPEED
//
//#include "ZXTestSupport.h"
#ifdef ZXING_BUILD_FOR_TEST
	#define ZXING_EXPORT_TEST_ONLY
	#define ZXING_IF_NOT_TEST(x)
#else
	#define ZXING_EXPORT_TEST_ONLY /* @sobolev static*/
	#define ZXING_IF_NOT_TEST(x) x
#endif
//
//#include "BitHacks.h"
#if __has_include(<bit>) && __cplusplus > 201703L // MSVC has the <bit> header but then warns about including it
	#include <bit>
	#if __cplusplus > 201703L && defined(__ANDROID__) // NDK 25.1.8937393 has the implementation but fails to advertise it
		#define __cpp_lib_bitops 201907L
	#endif
#endif
#if defined(__clang__) || defined(__GNUC__)
	#define ZX_HAS_GCC_BUILTINS
#elif defined(_MSC_VER) && !defined(_M_ARM) && !defined(_M_ARM64)
	#include <intrin.h>
	#define ZX_HAS_MSC_BUILTINS
#endif

namespace ZXing::BitHacks {
	/**
	 * The code below is taken from https://graphics.stanford.edu/~seander/bithacks.html
	 * All credits go to Sean Eron Anderson and other authors mentioned in that page.
	 */

	/// <summary>
	/// Compute the number of zero bits on the left.
	/// </summary>
	/* @sobolev (replaced with SBits::Clz) template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > > inline int NumberOfLeadingZeros(T x)
	{
	#ifdef __cpp_lib_bitops
		return std::countl_zero(static_cast<std::make_unsigned_t<T> >(x));
	#else
		if constexpr(sizeof(x) <= 4) {
			if(x == 0)
				return 32;
	#ifdef ZX_HAS_GCC_BUILTINS
			return __builtin_clz(x);
	#elif defined(ZX_HAS_MSC_BUILTINS)
			return __lzcnt(x);
	#else
			int n = 0;
			if((x & 0xFFFF0000) == 0) {
				n = n + 16; x = x << 16;
			}
			if((x & 0xFF000000) == 0) {
				n = n + 8; x = x << 8;
			}
			if((x & 0xF0000000) == 0) {
				n = n + 4; x = x << 4;
			}
			if((x & 0xC0000000) == 0) {
				n = n + 2; x = x << 2;
			}
			if((x & 0x80000000) == 0) {
				n = n + 1;
			}
			return n;
	#endif
		}
		else {
			if(x == 0)
				return 64;
	#ifdef ZX_HAS_GCC_BUILTINS
			return __builtin_clzll(x);
	#elif defined(ZX_HAS_MSC_BUILTINS)
			return __lzcnt64(x);
	#else
			int n = NumberOfLeadingZeros(static_cast<uint32_t>(x >> 32)); // @recursion
			if(n == 32)
				n += NumberOfLeadingZeros(static_cast<uint32_t>(x)); // @recursion
			return n;
	#endif
		}
	#endif
	}*/

	/// <summary>
	/// Compute the number of zero bits on the right.
	/// </summary>
	/* @sobolev (replaced with SBits::Ctz) template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > > inline int NumberOfTrailingZeros(T v)
	{
		assert(v != 0);
	#ifdef __cpp_lib_bitops
		return std::countr_zero(static_cast<std::make_unsigned_t<T> >(v));
	#else
		if constexpr(sizeof(v) <= 4) {
	#ifdef ZX_HAS_GCC_BUILTINS
			return __builtin_ctz(v);
	#elif defined(ZX_HAS_MSC_BUILTINS)
			unsigned long where;
			if(_BitScanForward(&where, v))
				return static_cast<int>(where);
			return 32;
	#else
			int c = 32;
			v &= -int32_t(v);
			if(v)  c--;
			if(v & 0x0000FFFF)  c -= 16;
			if(v & 0x00FF00FF)  c -= 8;
			if(v & 0x0F0F0F0F)  c -= 4;
			if(v & 0x33333333)  c -= 2;
			if(v & 0x55555555)  c -= 1;
			return c;
	#endif
		}
		else {
	#ifdef ZX_HAS_GCC_BUILTINS
			return __builtin_ctzll(v);
	#elif defined(ZX_HAS_MSC_BUILTINS)
			unsigned long where;
		#if defined(_WIN64)
			if(_BitScanForward64(&where, v))
				return static_cast<int>(where);
		#elif defined(_WIN32)
			if(_BitScanForward(&where, static_cast<unsigned long>(v)))
				return static_cast<int>(where);
			if(_BitScanForward(&where, static_cast<unsigned long>(v >> 32)))
				return static_cast<int>(where + 32);
		#else
			#error "Implementation of __builtin_ctzll required"
		#endif
			return 64;
	#else
			int n = NumberOfTrailingZeros(static_cast<uint32_t>(v)); // @recursion
			if(n == 32)
				n += NumberOfTrailingZeros(static_cast<uint32_t>(v >> 32)); // @recursion
			return n;
	#endif
		}
	#endif
	}*/
	inline uint32_t Reverse(uint32_t v)
	{
	#if 0
		return __builtin_bitreverse32(v);
	#else
		v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
		// swap consecutive pairs
		v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
		// swap nibbles ...
		v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
		// swap bytes
		v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
		// swap 2-byte long pairs
		v = (v >> 16) | (v << 16);
		return v;
	#endif
	}

	/* @v11.9.1 (replaced with SBits::Cpop) inline int CountBitsSet(uint32_t v)
	{
	#ifdef __cpp_lib_bitops
		return std::popcount(v);
	#elif defined(ZX_HAS_GCC_BUILTINS)
		return __builtin_popcount(v);
	#else
		v = v - ((v >> 1) & 0x55555555); // reuse input as temporary
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // temp
		return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
	#endif
	}*/

	// this is the same as log base 2 of v
	inline int HighestBitSet(uint32_t v) { return 31 - /*NumberOfLeadingZeros*/SBits::Clz(v); }

	// shift a whole array of bits by offset bits to the right (thinking of the array as a contiguous stream of bits
	// starting with the LSB of the first int and ending with the MSB of the last int, this is actually a left shift)
	template <typename T> void ShiftRight(std::vector<T>& bits, std::size_t offset)
	{
		assert(offset < sizeof(T) * 8);
		if(offset == 0 || bits.empty())
			return;
		std::size_t leftOffset = sizeof(T) * 8 - offset;
		for(std::size_t i = 0; i < bits.size() - 1; ++i) {
			bits[i] = (bits[i] >> offset) | (bits[i + 1] << leftOffset);
		}
		bits.back() >>= offset;
	}

	// reverse a whole array of bits. padding is the number of 'dummy' bits at the end of the array
	template <typename T> void Reverse(std::vector<T>& bits, std::size_t padding)
	{
		static_assert(sizeof(T) == sizeof(uint32_t), "Reverse only implemented for 32 bit types");

		// reverse all int's first (reversing the ints in the array and the bits in the ints at the same time)
		auto first = bits.begin(), last = bits.end();
		for(; first < --last; ++first) {
			auto t = *first;
			*first = BitHacks::Reverse(*last);
			*last = BitHacks::Reverse(t);
		}
		if(first == last)
			*last = BitHacks::Reverse(*last);

		// now correct the int's if the bit size isn't a multiple of 32
		ShiftRight(bits, padding);
	}

	// use to avoid "load of misaligned address" when using a simple type cast
	template <typename T> T LoadU(const void* ptr)
	{
		static_assert(std::is_integral<T>::value, "T must be an integer");
		T res;
		memcpy(&res, ptr, sizeof(T));
		return res;
	}
} // namespace ZXing::BitHacks
//#include "Flags.h"
namespace ZXing {
	//
	// @sobolev Note: Фактически, этот класс используется только для BarcodeFormat. Таким образом, правильнее его элиминировать вообще!
	//
	template<typename Enum> class Flags {
		static_assert(std::is_enum<Enum>::value, "Flags is only usable on enumeration types.");

		using Int = typename std::underlying_type<Enum>::type;
		Int i = 0;

		constexpr inline Flags(Int other) : i(other) {}
		constexpr static inline unsigned highestBitSet(Int x) noexcept { return x < 2 ? x : 1 + highestBitSet(x >> 1); }

	public:
		using enum_type = Enum;
		constexpr inline Flags() noexcept = default;
		constexpr inline Flags(Enum flag) noexcept : i(Int(flag)) {}
	//	constexpr inline Flags(std::initializer_list<Enum> flags) noexcept
	//		: i(initializer_list_helper(flags.begin(), flags.end()))
	//	{}
		class iterator {
			friend class Flags;
			const Int _flags = 0;
			int _pos = 0;
			iterator(Int i, int p) : _flags(i), _pos(p) {}
		public:
			using iterator_category = std::input_iterator_tag;
			using value_type = Enum;
			using difference_type = std::ptrdiff_t;
			using pointer = Enum*;
			using reference = Enum&;
			Enum operator*() const noexcept { return Enum(1 << _pos); }
			iterator& operator++() noexcept
			{
				while(++_pos < BitHacks::HighestBitSet(_flags) && !((1 << _pos) & _flags))
					;
				return *this;
			}
			bool operator==(const iterator& rhs) const noexcept { return _pos == rhs._pos; }
			bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }
		};

		iterator begin() const noexcept { return {i, /*BitHacks::NumberOfTrailingZeros*/static_cast<int>(SBits::Ctz(static_cast<uint>(i)))}; }
		iterator end() const noexcept { return {i, BitHacks::HighestBitSet(i) + 1}; }
		bool empty() const noexcept { return i == 0; }
		int count() const noexcept { return /*BitHacks::CountBitsSet*/SBits::Cpop(i); }
		constexpr inline bool operator==(Flags other) const noexcept { return i == other.i; }
		inline Flags& operator&=(Flags mask) noexcept { return i &= mask.i, *this; }
		inline Flags& operator&=(Enum mask) noexcept { return i &= Int(mask), *this; }
		inline Flags& operator|=(Flags other) noexcept { return i |= other.i, *this; }
		inline Flags& operator|=(Enum other) noexcept { return i |= Int(other), *this; }
	//	inline Flags &operator^=(Flags other) noexcept { return i ^= other.i, *this; }
	//	inline Flags &operator^=(Enum other) noexcept { return i ^= Int(other), *this; }

		constexpr inline Flags operator&(Flags other) const noexcept { return i & other.i; }
		constexpr inline Flags operator&(Enum other) const noexcept { return i & Int(other); }
		constexpr inline Flags operator|(Flags other) const noexcept { return i | other.i; }
		constexpr inline Flags operator|(Enum other) const noexcept { return i | Int(other); }
	//	constexpr inline Flags operator^(Flags other) const noexcept { return i ^ other.i; }
	//	constexpr inline Flags operator^(Enum other) const noexcept { return i ^ Int(other); }
	//	constexpr inline Flags operator~() const noexcept { return ~i; }

	//	constexpr inline operator Int() const noexcept { return i; }
	//	constexpr inline bool operator!() const noexcept { return !i; }
	//	constexpr inline Int asInt() const noexcept { return i; }

		constexpr inline bool testFlag(Enum flag) const noexcept
		{
			return (i & Int(flag)) == Int(flag) && (Int(flag) != 0 || i == Int(flag));
		}
		constexpr inline bool testFlags(Flags mask) const noexcept { return i & mask.i; }
		inline Flags& setFlag(Enum flag, bool on = true) noexcept
		{
			return on ? (*this |= flag) : (*this &= ~Int(flag));
		}
		inline void clear() noexcept { i = 0; }
		constexpr static Flags all() noexcept { return ~(unsigned(~0) << highestBitSet(Int(Enum::_max))); }
	private:
		//	constexpr static inline Int
		//	initializer_list_helper(typename std::initializer_list<Enum>::const_iterator it,
		//							typename std::initializer_list<Enum>::const_iterator end) noexcept
		//	{
		//		return (it == end ? Int(0) : (Int(*it) | initializer_list_helper(it + 1, end)));
		//	}
	};

	#define ZX_DECLARE_FLAGS(FLAGS, ENUM) \
		using FLAGS = Flags<ENUM>; \
		constexpr inline FLAGS operator|(FLAGS::enum_type e1, FLAGS::enum_type e2) noexcept { return FLAGS(e1) | e2; } \
		constexpr inline FLAGS operator|(FLAGS::enum_type e, FLAGS f) noexcept { return f | e; } \
		constexpr inline bool operator==(FLAGS::enum_type e, FLAGS f) noexcept { return FLAGS(e) == f; } \
		constexpr inline bool operator==(FLAGS f, FLAGS::enum_type e) noexcept { return FLAGS(e) == f; }
} // ZXing
//#include "BarcodeFormat.h"
namespace ZXing {
	/**
	* Enumerates barcode formats known to this package.
	*/
	enum class BarcodeFormat {
		// The values are an implementation detail. The c++ use-case (ZXing::Flags) could have been designed such that it
		// would not have been necessary to explicitly set the values to single bit constants. This has been done to ease
		// the interoperability with C-like interfaces, the python and the Qt wrapper.
		None            = 0,         ///< Used as a return value if no valid barcode has been detected
		Aztec           = (1 << 0),  ///< Aztec
		Codabar         = (1 << 1),  ///< Codabar
		Code39          = (1 << 2),  ///< Code39
		Code93          = (1 << 3),  ///< Code93
		Code128         = (1 << 4),  ///< Code128
		DataBar         = (1 << 5),  ///< GS1 DataBar, formerly known as RSS 14
		DataBarExpanded = (1 << 6),  ///< GS1 DataBar Expanded, formerly known as RSS EXPANDED
		DataMatrix      = (1 << 7),  ///< DataMatrix
		EAN8            = (1 << 8),  ///< EAN-8
		EAN13           = (1 << 9),  ///< EAN-13
		ITF             = (1 << 10), ///< ITF (Interleaved Two of Five)
		MaxiCode        = (1 << 11), ///< MaxiCode
		PDF417          = (1 << 12), ///< PDF417
		QRCode          = (1 << 13), ///< QR Code
		UPCA            = (1 << 14), ///< UPC-A
		UPCE            = (1 << 15), ///< UPC-E
		MicroQRCode     = (1 << 16), ///< Micro QR Code
		RMQRCode        = (1 << 17), ///< Rectangular Micro QR Code

		LinearCodes = Codabar | Code39 | Code93 | Code128 | EAN8 | EAN13 | ITF | DataBar | DataBarExpanded | UPCA | UPCE,
		MatrixCodes = Aztec | DataMatrix | MaxiCode | PDF417 | QRCode | MicroQRCode | RMQRCode,
		Any         = LinearCodes | MatrixCodes,

		_max = RMQRCode, ///> implementation detail, don't use
	};

	ZX_DECLARE_FLAGS(BarcodeFormats, BarcodeFormat)

	std::string ToString(BarcodeFormat format);
	std::string ToString(BarcodeFormats formats);

	/**
	 * @brief Parse a string into a BarcodeFormat. '-' and '_' are optional.
	 * @return None if str can not be parsed as a valid enum value
	 */
	BarcodeFormat BarcodeFormatFromString(std::string_view str);

	/**
	 * @brief Parse a string into a set of BarcodeFormats.
	 * Separators can be (any combination of) '|', ',' or ' '.
	 * Underscores are optional and input can be lower case.
	 * e.g. "EAN-8 qrcode, Itf" would be parsed into [EAN8, QRCode, ITF].
	 * @throws std::invalid_parameter if the string can not be fully parsed.
	 */
	BarcodeFormats BarcodeFormatsFromString(std::string_view str);
} // ZXing
//#include "ByteArray.h"
namespace ZXing {
	/**
			ByteArray is an extension of std::vector<uchar>.
	 */
	class ByteArray : public std::vector<uint8_t> {
	public:
		ByteArray() : std::vector<uint8_t>()
		{
		}
		ByteArray(std::initializer_list<uint8_t> list) : std::vector<uint8_t>(list) 
		{
		}
		explicit ByteArray(int len) : std::vector<uint8_t>(len, 0) 
		{
		}
		explicit ByteArray(const std::string& str) : std::vector<uint8_t>(str.begin(), str.end()) 
		{
		}
		void append(const ByteArray& other) 
		{
			insert(end(), other.begin(), other.end());
		}
		std::string_view asString(size_t pos = 0, size_t len = std::string_view::npos) const
		{
			return std::string_view(reinterpret_cast<const char*>(data()), size()).substr(pos, len);
		}
	};

	inline std::string ToHex(const ByteArray& bytes)
	{
		std::string res(bytes.size() * 3, ' ');
		for(size_t i = 0; i < bytes.size(); ++i) {
		#ifdef _MSC_VER
			sprintf_s(&res[i * 3], 4, "%02X ", bytes[i]);
		#else
			snprintf(&res[i * 3], 4, "%02X ", bytes[i]);
		#endif
		}
		return res.substr(0, res.size()-1);
	}
} // ZXing
//#include "CharacterSet.h"
namespace ZXing {
	enum class CharacterSet : uchar {
		Unknown,
		ASCII,
		ISO8859_1,
		ISO8859_2,
		ISO8859_3,
		ISO8859_4,
		ISO8859_5,
		ISO8859_6,
		ISO8859_7,
		ISO8859_8,
		ISO8859_9,
		ISO8859_10,
		ISO8859_11,
		ISO8859_13,
		ISO8859_14,
		ISO8859_15,
		ISO8859_16,
		Cp437,
		Cp1250,
		Cp1251,
		Cp1252,
		Cp1256,

		Shift_JIS,
		Big5,
		GB2312,
		GB18030,
		EUC_JP,
		EUC_KR,
		UTF16BE,
		UnicodeBig [[deprecated]] = UTF16BE,
		UTF8,
		UTF16LE,
		UTF32BE,
		UTF32LE,
		BINARY,
		CharsetCount
	};

	CharacterSet CharacterSetFromString(std::string_view name);
	std::string ToString(CharacterSet cs);
} // ZXing
//#include "DecodeHints.h"
namespace ZXing {
	/**
	 * @brief The Binarizer enum
	 *
	 * Specify which algorithm to use for the grayscale to binary transformation.
	 * The difference is how to get to a threshold value T which results in a bit
	 * value R = L <= T.
	 */
	enum class Binarizer : uchar { // needs to be unsigned for the bitfield below to work, uint8_t fails as well
		LocalAverage,    ///< T = average of neighboring pixels for matrix and GlobalHistogram for linear (HybridBinarizer)
		GlobalHistogram, ///< T = valley between the 2 largest peaks in the histogram (per line in linear case)
		FixedThreshold,  ///< T = 127
		BoolCast,        ///< T = 0, fastest possible
	};

	enum class EanAddOnSymbol : uchar { // see above
		Ignore,  ///< Ignore any Add-On symbol during read/scan
		Read,    ///< Read EAN-2/EAN-5 Add-On symbol if found
		Require, ///< Require EAN-2/EAN-5 Add-On symbol to be present
	};

	enum class TextMode : uchar { // see above
		Plain,   ///< bytes() transcoded to unicode based on ECI info or guessed charset (the default mode prior to 2.0)
		ECI,     ///< standard content following the ECI protocol with every character set ECI segment transcoded to unicode
		HRI,     ///< Human Readable Interpretation (dependent on the ContentType)
		Hex,     ///< bytes() transcoded to ASCII string of HEX values
		Escaped, ///< Use the EscapeNonGraphical() function (e.g. ASCII 29 will be transcoded to "<GS>")
	};

	class DecodeHints {
		bool _tryHarder/* : 1*/;
		bool _tryRotate/* : 1*/;
		bool _tryInvert/*: 1*/;
		bool _tryDownscale/*: 1*/;
		bool _isPure/*: 1*/;
		bool _tryCode39ExtendedMode/*: 1*/;
		bool _validateCode39CheckSum/*: 1*/;
		bool _validateITFCheckSum/*: 1*/;
		bool _returnCodabarStartEnd/*: 1*/;
		bool _returnErrors/*: 1*/;
		uint8_t _downscaleFactor/*: 3*/;
		EanAddOnSymbol _eanAddOnSymbol/*: 2*/;
		Binarizer _binarizer/*: 2*/;
		TextMode _textMode/*: 3*/;
		CharacterSet _characterSet/*: 6*/;
	#ifdef ZXING_BUILD_EXPERIMENTAL_API
		bool _tryDenoise/*: 1*/;
	#else
		bool Reserve; // @alignment
	#endif
		uint8_t _minLineCount        = 2;
		uint8_t _maxNumberOfSymbols  = 0xff;
		uint16_t _downscaleThreshold = 500;
		BarcodeFormats _formats      = BarcodeFormat::None;
	public:
		// bitfields don't get default initialized to 0 before c++20
		DecodeHints() : _tryHarder(true), _tryRotate(true), _tryInvert(true), _tryDownscale(true), _isPure(false),
			  _tryCode39ExtendedMode(false), _validateCode39CheckSum(false), _validateITFCheckSum(false),
			  _returnCodabarStartEnd(false), _returnErrors(false), _downscaleFactor(3),
			  _eanAddOnSymbol(EanAddOnSymbol::Ignore), _binarizer(Binarizer::LocalAverage),
			  _textMode(TextMode::HRI), _characterSet(CharacterSet::Unknown)
	#ifdef ZXING_BUILD_EXPERIMENTAL_API
			  , _tryDenoise(0)
	#endif
		{}

	#define ZX_PROPERTY(TYPE, GETTER, SETTER) \
		TYPE GETTER() const noexcept { return _##GETTER; } \
		DecodeHints& SETTER(TYPE v)& { return _##GETTER = std::move(v), *this; } \
		DecodeHints&& SETTER(TYPE v)&& { return _##GETTER = std::move(v), std::move(*this); }

		/// Specify a set of BarcodeFormats that should be searched for, the default is all supported formats.
		ZX_PROPERTY(BarcodeFormats, formats, setFormats)
		/// Spend more time to try to find a barcode; optimize for accuracy, not speed.
		ZX_PROPERTY(bool, tryHarder, setTryHarder)
		/// Also try detecting code in 90, 180 and 270 degree rotated images.
		ZX_PROPERTY(bool, tryRotate, setTryRotate)
		/// Also try detecting inverted ("reversed reflectance") codes if the format allows for those.
		ZX_PROPERTY(bool, tryInvert, setTryInvert)
		/// Also try detecting code in downscaled images (depending on image size).
		ZX_PROPERTY(bool, tryDownscale, setTryDownscale)
	#ifdef ZXING_BUILD_EXPERIMENTAL_API
		/// Also try detecting code after denoising (currently morphological closing filter for 2D symbologies only).
		ZX_PROPERTY(bool, tryDenoise, setTryDenoise)
	#endif
		/// Binarizer to use internally when using the ReadBarcode function
		ZX_PROPERTY(Binarizer, binarizer, setBinarizer)
		/// Set to true if the input contains nothing but a single perfectly aligned barcode (generated image)
		ZX_PROPERTY(bool, isPure, setIsPure)
		/// Image size ( min(width, height) ) threshold at which to start downscaled scanning
		// WARNING: this API is experimental and may change/disappear
		ZX_PROPERTY(uint16_t, downscaleThreshold, setDownscaleThreshold)
		/// Scale factor used during downscaling, meaningful values are 2, 3 and 4
		// WARNING: this API is experimental and may change/disappear
		ZX_PROPERTY(uint8_t, downscaleFactor, setDownscaleFactor)
		/// The number of scan lines in a linear barcode that have to be equal to accept the result, default is 2
		ZX_PROPERTY(uint8_t, minLineCount, setMinLineCount)
		/// The maximum number of symbols (barcodes) to detect / look for in the image with ReadBarcodes
		ZX_PROPERTY(uint8_t, maxNumberOfSymbols, setMaxNumberOfSymbols)
		/// If true, the Code-39 reader will try to read extended mode.
		ZX_PROPERTY(bool, tryCode39ExtendedMode, setTryCode39ExtendedMode)
		/// Assume Code-39 codes employ a check digit and validate it.
		ZX_PROPERTY(bool, validateCode39CheckSum, setValidateCode39CheckSum)
		/// Assume ITF codes employ a GS1 check digit and validate it.
		ZX_PROPERTY(bool, validateITFCheckSum, setValidateITFCheckSum)
		/// If true, return the start and end chars in a Codabar barcode instead of stripping them.
		ZX_PROPERTY(bool, returnCodabarStartEnd, setReturnCodabarStartEnd)
		/// If true, return the barcodes with errors as well (e.g. checksum errors, see @Result::error())
		ZX_PROPERTY(bool, returnErrors, setReturnErrors)
		/// Specify whether to ignore, read or require EAN-2/5 add-on symbols while scanning EAN/UPC codes
		ZX_PROPERTY(EanAddOnSymbol, eanAddOnSymbol, setEanAddOnSymbol)
		/// Specifies the TextMode that controls the return of the Result::text() function
		ZX_PROPERTY(TextMode, textMode, setTextMode)
		/// Specifies fallback character set to use instead of auto-detecting it (when applicable)
		ZX_PROPERTY(CharacterSet, characterSet, setCharacterSet)
		DecodeHints& setCharacterSet(std::string_view v)& { return _characterSet = CharacterSetFromString(v), *this; }
		DecodeHints&& setCharacterSet(std::string_view v) && { return _characterSet = CharacterSetFromString(v), std::move(*this); }
	#undef ZX_PROPERTY
		bool hasFormat(BarcodeFormats f) const noexcept { return _formats.testFlags(f) || _formats.empty(); }
	};
} // ZXing
//#include "Content.h"
namespace ZXing {
	enum class ECI : int;
	enum class ContentType { Text, Binary, Mixed, GS1, ISO15434, UnknownECI };
	enum class AIFlag : char { None, GS1, AIM };

	std::string ToString(ContentType type);

	struct SymbologyIdentifier {
		SymbologyIdentifier() : code(0), modifier(0), eciModifierOffset(0), aiFlag(AIFlag::None)
		{
		}
		SymbologyIdentifier(char aCode, char aModif, char aEciModifOffs) : code(aCode), modifier(aModif), eciModifierOffset(aEciModifOffs), aiFlag(AIFlag::None)
		{
		}
		SymbologyIdentifier(char aCode, char aModif, char aEciModifOffs, AIFlag aif) : 
			code(aCode), modifier(aModif), eciModifierOffset(aEciModifOffs), aiFlag(aif)
		{
		}
		char   code;
		char   modifier;
		char   eciModifierOffset;
		AIFlag aiFlag;
		std::string toString(bool hasECI = false) const
		{
			return code ? ']' + std::string(1, code) + static_cast<char>(modifier + eciModifierOffset * hasECI) : std::string();
		}
	};

	class Content {
		template <typename FUNC> void ForEachECIBlock(FUNC f) const;
		void switchEncoding(ECI eci, bool isECI);
		std::string render(bool withECI) const;
	public:
		struct Encoding {
			ECI eci;
			int pos;
		};
		ByteArray bytes;
		std::vector<Encoding> encodings;
		SymbologyIdentifier symbology;
		CharacterSet defaultCharset; // 1byte
		bool   hasECI;
		uint8  Reserve[2]; // @alignment

		Content();
		Content(ByteArray&& bytes, SymbologyIdentifier si);
		void switchEncoding(ECI eci) { switchEncoding(eci, true); }
		void switchEncoding(CharacterSet cs);
		void reserve(int count) { bytes.reserve(bytes.size() + count); }
		void push_back(uint8_t val) { bytes.push_back(val); }
		void append(const std::string& str) { bytes.insert(bytes.end(), str.begin(), str.end()); }
		void append(const ByteArray& ba) { bytes.insert(bytes.end(), ba.begin(), ba.end()); }
		void append(const Content& other);
		void operator+=(char val) { push_back(val); }
		void operator+=(const std::string& str) { append(str); }
		void erase(int pos, int n);
		void insert(int pos, const std::string& str);
		bool empty() const { return bytes.empty(); }
		bool canProcess() const;
		std::string text(TextMode mode) const;
		std::wstring utfW() const; // utf16 or utf32 depending on the platform, i.e. on size_of(wchar_t)
		std::string utf8() const { return render(false); }
		ByteArray bytesECI() const;
		CharacterSet guessEncoding() const;
		ContentType type() const;
	};
} // ZXing
//#include "Error.h"
namespace ZXing {
	class Error {
	public:
		enum class Type : uint8_t { None, Format, Checksum, Unsupported };

		Type type() const noexcept { return _type; }
		const std::string& msg() const noexcept { return _msg; }
		explicit operator bool() const noexcept { return _type != Type::None; }
		std::string location() const
		{
			if(!_file)
				return {};
			std::string file(_file);
			return file.substr(file.find_last_of("/\\") + 1) + ":" + std::to_string(_line);
		}
		Error() : _file(0), _line(-1), _type(Type::None)
		{
		}
		Error(Type type, std::string msg = {}) : _msg(std::move(msg)), _file(0), _line(-1), _type(type)
		{
		}
		Error(const char* file, short line, Type type, std::string msg = {}) : _msg(std::move(msg)), _file(file), _line(line), _type(type) 
		{
		}
		static constexpr auto Format = Type::Format;
		static constexpr auto Checksum = Type::Checksum;
		static constexpr auto Unsupported = Type::Unsupported;
		inline bool operator==(const Error& o) const noexcept
		{
			return (_type == o._type && _msg == o._msg && _file == o._file && _line == o._line);
		}
		inline bool operator!=(const Error& o) const noexcept { return !(*this == o); }
	protected:
		std::string _msg;
		const char* _file = nullptr;
		short _line = -1;
		Type _type = Type::None;
	};

	inline bool operator==(const Error& e, Error::Type t) noexcept { return e.type() == t; }
	inline bool operator!=(const Error& e, Error::Type t) noexcept { return !(e == t); }
	inline bool operator==(Error::Type t, const Error& e) noexcept { return e.type() == t; }
	inline bool operator!=(Error::Type t, const Error& e) noexcept { return !(t == e); }

	#define FormatError(...) Error(__FILE__, __LINE__, Error::Format, std::string(__VA_ARGS__))
	#define ChecksumError(...) Error(__FILE__, __LINE__, Error::Checksum, std::string(__VA_ARGS__))
	#define UnsupportedError(...) Error(__FILE__, __LINE__, Error::Unsupported, std::string(__VA_ARGS__))

	inline std::string ToString(const Error& e)
	{
		const char* name[] = {"", "FormatError", "ChecksumError", "Unsupported"};
		std::string ret = name[static_cast<int>(e.type())];
		if(!e.msg().empty())
			ret += " (" + e.msg() + ")";
		if(auto location = e.location(); !location.empty())
			ret += " @ " + e.location();
		return ret;
	}
}
//#include "Point.h"
namespace ZXing {
	template <typename T> struct PointT {
		using value_t = T;
		T x = 0;
		T y = 0;

		constexpr PointT() = default;
		constexpr PointT(T x, T y) : x(x), y(y) {}

		template <typename U> constexpr explicit PointT(const PointT<U>& p) : x(static_cast<T>(p.x)), y(static_cast<T>(p.y))
		{
		}
		template <typename U> PointT& operator+=(const PointT<U>& b)
		{
			x += b.x;
			y += b.y;
			return *this;
		}
	};

	template <typename T> bool operator==(const PointT<T>& a, const PointT<T>& b) { return a.x == b.x && a.y == b.y; }
	template <typename T> bool operator!=(const PointT<T>& a, const PointT<T>& b) { return !(a == b); }
	template <typename T> auto operator-(const PointT<T>& a) -> PointT<T> { return {-a.x, -a.y}; }
	template <typename T, typename U> auto operator+(const PointT<T>& a, const PointT<U>& b) -> PointT<decltype(a.x + b.x)> { return {a.x + b.x, a.y + b.y}; }
	template <typename T, typename U> auto operator-(const PointT<T>& a, const PointT<U>& b) -> PointT<decltype(a.x - b.x)> { return {a.x - b.x, a.y - b.y}; }
	template <typename T, typename U> auto operator*(const PointT<T>& a, const PointT<U>& b) -> PointT<decltype(a.x * b.x)> { return {a.x * b.x, a.y * b.y}; }
	template <typename T, typename U> PointT<T> operator*(U s, const PointT<T>& a) { return {s * a.x, s * a.y}; }
	template <typename T, typename U> PointT<T> operator/(const PointT<T>& a, U d) { return {a.x / d, a.y / d}; }
	template <typename T, typename U> auto dot(const PointT<T>& a, const PointT<U>& b) -> decltype (a.x * b.x) { return a.x * b.x + a.y * b.y; }
	template <typename T> auto cross(PointT<T> a, PointT<T> b) -> decltype(a.x * b.x) { return a.x * b.y - b.x * a.y; }

	/// L1 norm
	template <typename T> T sumAbsComponent(PointT<T> p) { return std::abs(p.x) + std::abs(p.y); }
	/// L2 norm
	template <typename T> auto length(PointT<T> p) -> decltype(std::sqrt(dot(p, p))) { return std::sqrt(dot(p, p)); }
	/// L-inf norm
	template <typename T> T maxAbsComponent(PointT<T> p) { return smax(std::abs(p.x), std::abs(p.y)); }
	template <typename T> auto distance(PointT<T> a, PointT<T> b) -> decltype(length(a - b)) { return length(a - b); }

	using PointI = PointT<int>;
	using PointF = PointT<double>;

	/// Calculate a floating point pixel coordinate representing the 'center' of the pixel.
	/// This is sort of the inverse operation of the PointI(PointF) conversion constructor.
	/// See also the documentation of the GridSampler API.
	inline PointF centered(PointI p) { return p + PointF(0.5f, 0.5f); }
	inline PointF centered(PointF p) { return {std::floor(p.x) + 0.5f, std::floor(p.y) + 0.5f}; }
	template <typename T> PointF normalized(PointT<T> d) { return PointF(d) / length(PointF(d)); }
	template <typename T> PointT<T> bresenhamDirection(PointT<T> d) { return d / maxAbsComponent(d); }
	template <typename T> PointT<T> mainDirection(PointT<T> d) { return std::abs(d.x) > std::abs(d.y) ? PointT<T>(d.x, 0) : PointT<T>(0, d.y); }
} // ZXing
//#include "ZXAlgorithms.h"
namespace ZXing {
	template <class T, class U> constexpr T narrow_cast(U&& u) noexcept { return static_cast<T>(std::forward<U>(u)); }
	template <typename Container, typename Value> auto Find(Container& c, const Value& v)->decltype(std::begin(c)) 
	{
		return std::find(std::begin(c), std::end(c), v);
	}
	template <typename Container, typename Predicate> auto FindIf(Container& c, Predicate p)->decltype(std::begin(c)) 
	{
		return std::find_if(std::begin(c), std::end(c), p);
	}
	template <typename Container, typename Value> auto Contains(const Container& c, const Value& v)->decltype(std::begin(c), bool ())
	{
		return Find(c, v) != std::end(c);
	}
	template <typename ListType, typename Value> auto Contains(const std::initializer_list<ListType>& c, const Value& v)->decltype(std::begin(c), bool ())
	{
		return Find(c, v) != std::end(c);
	}
	inline bool Contains(const char* str, char c) { return strchr(str, c) != nullptr; }
	template <template <typename ...> typename C, typename ... Ts>
	auto FirstOrDefault(C<Ts ...>&& results)
	{
		return results.empty() ? typename C<Ts ...>::value_type() : std::move(results.front());
	}
	template <typename Container, typename Value = typename Container::value_type, typename Op = std::plus<Value> >
	Value Reduce(const Container& c, Value v = Value{}, Op op = {}) {
		return std::accumulate(std::begin(c), std::end(c), v, op);
	}
	// see C++20 ssize
	template <class Container> constexpr auto Size(const Container& c)->decltype(c.size(), int ()) { return narrow_cast<int>(c.size()); }
	template <class T, std::size_t N> constexpr int Size(const T (&)[N]) noexcept { return narrow_cast<int>(N); }
	template <typename Container, typename Value> int IndexOf(const Container& c, const Value& v) 
	{
		auto i = Find(c, v);
		return i == std::end(c) ? -1 : narrow_cast<int>(std::distance(std::begin(c), i));
	}
	inline int IndexOf(const char* str, char c) 
	{
		auto s = strchr(str, c);
		return s != nullptr ? narrow_cast<int>(s - str) : -1;
	}
	template <typename Container, typename Value, class UnaryOp> Value TransformReduce(const Container& c, Value s, UnaryOp op) 
	{
		for(const auto & v : c)
			s += op(v);
		return s;
	}
	template <typename T = char> T ToDigit(int i)
	{
		if(i < 0 || i > 9)
			throw FormatError("Invalid digit value");
		return static_cast<T>('0' + i);
	}
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > > std::string ToString(T val, int len)
	{
		std::string result(len--, '0');
		if(val < 0)
			throw FormatError("Invalid value");
		for(; len >= 0 && val != 0; --len, val /= 10)
			result[len] = '0' + val % 10;
		if(val)
			throw FormatError("Invalid value");
		return result;
	}
	template <typename T> void UpdateMin(T& min, T val) { min = smin(min, val); }
	template <typename T> void UpdateMax(T& max, T val) { max = smax(max, val); }
	template <typename T> void UpdateMinMax(T& min, T& max, T val)
	{
		min = smin(min, val);
		max = smax(max, val);

		// Note: the above code is not equivalent to
		//    if (val < min)        min = val;
		//    else if (val > max)   max = val;
		// It is basically the same but without the 'else'. For the 'else'-variant to work,
		// both min and max have to be initialized with a value that is part of the sequence.
		// Also it turns out clang and gcc can vectorize the code above but not the code below.
	}
} // ZXing
//#include "Quadrilateral.h"
namespace ZXing {
	template <typename T> class Quadrilateral : public std::array<T, 4> {
		using Base = std::array<T, 4>;
		using Base::at;
	public:
		using Point = T;
		Quadrilateral() = default;
		Quadrilateral(T tl, T tr, T br, T bl) : Base{tl, tr, br, bl} 
		{
		}
		template <typename U> Quadrilateral(PointT<U> tl, PointT<U> tr, PointT<U> br, PointT<U> bl) : Quadrilateral(Point(tl), Point(tr), Point(br), Point(bl))
		{
		}
		constexpr Point topLeft() const noexcept { return at(0); }
		constexpr Point topRight() const noexcept { return at(1); }
		constexpr Point bottomRight() const noexcept { return at(2); }
		constexpr Point bottomLeft() const noexcept { return at(3); }
		double orientation() const
		{
			auto centerLine = (topRight() + bottomRight()) - (topLeft() + bottomLeft());
			if(centerLine == Point{})
				return 0.;
			auto centerLineF = normalized(centerLine);
			return std::atan2(centerLineF.y, centerLineF.x);
		}
	};

	using QuadrilateralF = Quadrilateral<PointF>;
	using QuadrilateralI = Quadrilateral<PointI>;

	template <typename PointT = PointF>
	Quadrilateral<PointT> Rectangle(int width, int height, typename PointT::value_t margin = 0)
	{
		return {PointT{margin, margin}, {width - margin, margin}, {width - margin, height - margin}, {margin, height - margin}};
	}
	template <typename PointT = PointF> Quadrilateral<PointT> CenteredSquare(int size)
	{
		return Scale(Quadrilateral(PointT{-1, -1}, {1, -1}, {1, 1}, {-1, 1}), size / 2);
	}
	template <typename PointT = PointI> Quadrilateral<PointT> Line(int y, int xStart, int xStop)
	{
		return {PointT{xStart, y}, {xStop, y}, {xStop, y}, {xStart, y}};
	}
	template <typename PointT> bool IsConvex(const Quadrilateral<PointT>& poly)
	{
		const int N = Size(poly);
		bool sign = false;
		typename PointT::value_t m = INFINITY, M = 0;
		for(int i = 0; i < N; i++) {
			auto d1 = poly[(i + 2) % N] - poly[(i + 1) % N];
			auto d2 = poly[i] - poly[(i + 1) % N];
			auto cp = cross(d1, d2);

			// TODO: see if the isInside check for all boundary points in GridSampler is still required after fixing
			// the wrong fabs()
			// application in the following line
			UpdateMinMax(m, M, std::fabs(cp));

			if(i == 0)
				sign = cp > 0;
			else if(sign != (cp > 0))
				return false;
		}
		// It turns out being convex is not enough to prevent a "numerical instability"
		// that can cause the corners being projected inside the image boundaries but
		// some points near the corners being projected outside. This has been observed
		// where one corner is almost in line with two others. The M/m ratio is below 2
		// for the complete existing sample set. For very "skewed" QRCodes a value of
		// around 3 is realistic. A value of 14 has been observed to trigger the
		// instability.
		return M / m < 4.0;
	}
	template <typename PointT> Quadrilateral<PointT> Scale(const Quadrilateral<PointT>& q, int factor)
	{
		return {factor * q[0], factor * q[1], factor * q[2], factor * q[3]};
	}
	template <typename PointT> PointT Center(const Quadrilateral<PointT>& q)
	{
		return Reduce(q) / Size(q);
	}
	template <typename PointT> Quadrilateral<PointT> RotatedCorners(const Quadrilateral<PointT>& q, int n = 1, bool mirror = false)
	{
		Quadrilateral<PointT> res;
		std::rotate_copy(q.begin(), q.begin() + ((n + 4) % 4), q.end(), res.begin());
		if(mirror)
			std::swap(res[1], res[3]);
		return res;
	}
	template <typename PointT> bool IsInside(const PointT& p, const Quadrilateral<PointT>& q)
	{
		// Test if p is on the same side (right or left) of all polygon segments
		int pos = 0, neg = 0;
		for(int i = 0; i < Size(q); ++i)
			(cross(p - q[i], q[(i + 1) % Size(q)] - q[i]) < 0 ? neg : pos)++;
		return pos == 0 || neg == 0;
	}

	template <typename PointT> Quadrilateral<PointT> BoundingBox(const Quadrilateral<PointT>& q)
	{
		auto [minX, maxX] = std::minmax({q[0].x, q[1].x, q[2].x, q[3].x});
		auto [minY, maxY] = std::minmax({q[0].y, q[1].y, q[2].y, q[3].y});
		return {PointT{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}};
	}

	template <typename PointT> bool HaveIntersectingBoundingBoxes(const Quadrilateral<PointT>& a, const Quadrilateral<PointT>& b)
	{
		auto bba = BoundingBox(a);
		auto bbb = BoundingBox(b);
		bool x = bbb.topRight().x < bba.topLeft().x || bbb.topLeft().x > bba.topRight().x;
		bool y = bbb.bottomLeft().y < bba.topLeft().y || bbb.topLeft().y > bba.bottomLeft().y;
		return !(x || y);
	}

	template <typename PointT> Quadrilateral<PointT> Blend(const Quadrilateral<PointT>& a, const Quadrilateral<PointT>& b)
	{
		auto dist2First = [c = a[0]](auto a, auto b) { return distance(a, c) < distance(b, c); };
		// rotate points such that the the two topLeft points are closest to each other
		auto offset = std::min_element(b.begin(), b.end(), dist2First) - b.begin();
		Quadrilateral<PointT> res;
		for(int i = 0; i < 4; ++i)
			res[i] = (a[i] + b[(i + offset) % 4]) / 2;
		return res;
	}
} // ZXing
//#include "StructuredAppend.h"
namespace ZXing {
	struct StructuredAppendInfo {
		int index = -1;
		int count = -1;
		std::string id;
	};
} // ZXing
//#include "Result.h"
namespace ZXing {
	class DecoderResult;
	class ImageView;

	using Position = QuadrilateralI;

	/**
	 * @brief The Result class encapsulates the result of decoding a barcode within an image.
	 */
	class Result {
		void setIsInverted(bool v) { _isInverted = v; }
		Result& setDecodeHints(DecodeHints hints);
		friend Result MergeStructuredAppendSequence(const std::vector<Result>& results);
		friend std::vector<Result> ReadBarcodes(const ImageView&, const DecodeHints&);
		friend void IncrementLineCount(Result&);
	public:
		Result() = default;
		// linear symbology convenience constructor
		Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format, SymbologyIdentifier si, Error error = {}, bool readerInit = false);
		Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format);
		bool isValid() const;
		const Error& error() const { return _error; }
		BarcodeFormat format() const { return _format; }
		/**
		 * @brief bytes is the raw / standard content without any modifications like character set conversions
		 */
		const ByteArray & bytes() const;
		/**
		 * @brief bytesECI is the raw / standard content following the ECI protocol
		 */
		ByteArray bytesECI() const;
		/**
		 * @brief text returns the bytes() content rendered to unicode/utf8 text accoring to specified TextMode
		 */
		std::string text(TextMode mode) const;
		/**
		 * @brief text returns the bytes() content rendered to unicode/utf8 text accoring to the TextMode set in the DecodingHints
		 */
		std::string text() const;
		/**
		 * @brief ecLevel returns the error correction level of the symbol (empty string if not applicable)
		 */
		/*std::string*/const char * ecLevel() const;
		/**
		 * @brief contentType gives a hint to the type of content found (Text/Binary/GS1/etc.)
		 */
		ContentType contentType() const;
		/**
		 * @brief hasECI specifies wheter or not an ECI tag was found
		 */
		bool hasECI() const;
		const Position & position() const { return _position; }
		void setPosition(Position pos) { _position = pos; }
		/**
		 * @brief orientation of barcode in degree, see also Position::orientation()
		 */
		int orientation() const;
		/**
		 * @brief isMirrored is the symbol mirrored (currently only supported by QRCode and DataMatrix)
		 */
		bool isMirrored() const { return _isMirrored; }
		/**
		 * @brief isInverted is the symbol inverted / has reveresed reflectance (see DecodeHints::tryInvert)
		 */
		bool isInverted() const { return _isInverted; }
		/**
		 * @brief symbologyIdentifier Symbology identifier "]cm" where "c" is symbology code character, "m" the modifier.
		 */
		std::string symbologyIdentifier() const;
		/**
		 * @brief sequenceSize number of symbols in a structured append sequence.
		 *
		 * If this is not part of a structured append sequence, the returned value is -1.
		 * If it is a structured append symbol but the total number of symbols is unknown, the
		 * returned value is 0 (see PDF417 if optional "Segment Count" not given).
		 */
		int sequenceSize() const;
		/**
		 * @brief sequenceIndex the 0-based index of this symbol in a structured append sequence.
		 */
		int sequenceIndex() const;
		/**
		 * @brief sequenceId id to check if a set of symbols belongs to the same structured append sequence.
		 *
		 * If the symbology does not support this feature, the returned value is empty (see MaxiCode).
		 * For QR Code, this is the parity integer converted to a string.
		 * For PDF417 and DataMatrix, this is the "fileId".
		 */
		std::string sequenceId() const;
		bool isLastInSequence() const { return sequenceSize() == sequenceIndex() + 1; }
		bool isPartOfSequence() const { return sequenceSize() > -1 && sequenceIndex() > -1; }
		/**
		 * @brief readerInit Set if Reader Initialisation/Programming symbol.
		 */
		bool readerInit() const { return _readerInit; }
		/**
		 * @brief lineCount How many lines have been detected with this code (applies only to linear symbologies)
		 */
		int lineCount() const { return _lineCount; }
		/**
		 * @brief version QRCode / DataMatrix / Aztec version or size.
		 */
		/*std::string*/const char * version() const;
		bool operator==(const Result& o) const;
	private:
		Content _content;
		Error _error;
		Position _position;
		DecodeHints _decodeHints;
		StructuredAppendInfo _sai;
		BarcodeFormat _format = BarcodeFormat::None;
		char _ecLevel[4] = {};
		char _version[4] = {};
		int _lineCount = 0;
		bool _isMirrored = false;
		bool _isInverted = false;
		bool _readerInit = false;
		uint8 Reserve; // @alignment
	};

	using Results = std::vector<Result>;
	/**
	 * @brief Merge a list of Results from one Structured Append sequence to a single result
	 */
	Result MergeStructuredAppendSequence(const Results& results);
	/**
	 * @brief Automatically merge all Structured Append sequences found in the given results
	 */
	Results MergeStructuredAppendSequences(const Results& results);
} // ZXing
//#include "MultiFormatReader.h"
namespace ZXing {
	class Result;
	class Reader;
	class BinaryBitmap;
	class DecodeHints;

	/**
	* MultiFormatReader is a convenience class and the main entry point into the library for most uses.
	* By default it attempts to decode all barcode formats that the library supports. Optionally, you
	* can provide a hints object to request different behavior, for example only decoding QR codes.
	*
	* @author Sean Owen
	* @author dswitkin@google.com (Daniel Switkin)
	*/
	class MultiFormatReader {
	public:
		explicit MultiFormatReader(const DecodeHints& hints);
		explicit MultiFormatReader(DecodeHints&& hints) = delete;
		~MultiFormatReader();
		Result read(const BinaryBitmap& image) const;
		// WARNING: this API is experimental and may change/disappear
		Results readMultiple(const BinaryBitmap& image, int maxSymbols = 0xFF) const;
	private:
		std::vector<std::unique_ptr<Reader>> _readers;
		const DecodeHints& _hints;
	};
} // ZXing
//#include "ImageView.h"
namespace ZXing {
	enum class ImageFormat : uint32_t {
		None = 0,
		Lum  = 0x01000000,
		RGB  = 0x03000102,
		BGR  = 0x03020100,
		RGBX = 0x04000102,
		XRGB = 0x04010203,
		BGRX = 0x04020100,
		XBGR = 0x04030201,
	};

	constexpr inline int PixStride(ImageFormat format) { return (static_cast<uint32_t>(format) >> 3*8) & 0xFF; }
	constexpr inline int RedIndex(ImageFormat format) { return (static_cast<uint32_t>(format) >> 2*8) & 0xFF; }
	constexpr inline int GreenIndex(ImageFormat format) { return (static_cast<uint32_t>(format) >> 1*8) & 0xFF; }
	constexpr inline int BlueIndex(ImageFormat format) { return (static_cast<uint32_t>(format) >> 0*8) & 0xFF; }

	constexpr inline uint8_t RGBToLum(uint r, uint g, uint b)
	{
		// .299R + 0.587G + 0.114B (YUV/YIQ for PAL and NTSC),
		// (306*R) >> 10 is approximately equal to R*0.299, and so on.
		// 0x200 >> 10 is 0.5, it implements rounding.
		return static_cast<uint8_t>((306 * r + 601 * g + 117 * b + 0x200) >> 10);
	}

	/**
	 * Simple class that stores a non-owning const pointer to image data plus layout and format information.
	 */
	class ImageView {
	protected:
		const uint8_t * _data = nullptr;
		ImageFormat _format;
		int _width = 0, _height = 0, _pixStride = 0, _rowStride = 0;
	public:
		/**
		 * ImageView constructor
		 *
		 * @param data  pointer to image buffer
		 * @param width  image width in pixels
		 * @param height  image height in pixels
		 * @param format  image/pixel format
		 * @param rowStride  optional row stride in bytes, default is width * pixStride
		 * @param pixStride  optional pixel stride in bytes, default is calculated from format
		 */
		ImageView(const uint8_t* data, int width, int height, ImageFormat format, int rowStride = 0, int pixStride = 0)
			: _data(data), _format(format), _width(width), _height(height),
			_pixStride(pixStride ? pixStride : PixStride(format)), _rowStride(rowStride ? rowStride : width * _pixStride)
		{}

		int width() const { return _width; }
		int height() const { return _height; }
		int pixStride() const { return _pixStride; }
		int rowStride() const { return _rowStride; }
		ImageFormat format() const { return _format; }
		const uint8_t * data(int x, int y) const { return _data + y * _rowStride + x * _pixStride; }
		ImageView cropped(int left, int top, int width, int height) const
		{
			left   = smax(0, left);
			top    = smax(0, top);
			width  = width <= 0 ? (_width - left) : smin(_width - left, width);
			height = height <= 0 ? (_height - top) : smin(_height - top, height);
			return {data(left, top), width, height, _format, _rowStride, _pixStride};
		}
		ImageView rotated(int degree) const
		{
			switch((degree + 360) % 360) {
				case 90:  return {data(0, _height - 1), _height, _width, _format, _pixStride, -_rowStride};
				case 180: return {data(_width - 1, _height - 1), _width, _height, _format, -_rowStride, -_pixStride};
				case 270: return {data(_width - 1, 0), _height, _width, _format, -_pixStride, _rowStride};
			}
			return *this;
		}
		ImageView subsampled(int scale) const
		{
			return {_data, _width / scale, _height / scale, _format, _rowStride * scale, _pixStride * scale};
		}
	};
} // ZXing
//#include "BinaryBitmap.h"
namespace ZXing {
	class BitMatrix;

	using PatternRow = std::vector<uint16_t>;

	/**
	* This class is the core bitmap class used by ZXing to represent 1 bit data. Reader objects
	* accept a BinaryBitmap and attempt to decode it.
	*/
	class BinaryBitmap {
		struct Cache;
		std::unique_ptr<Cache> _cache;
		bool _inverted = false;
		bool _closed = false;
	protected:
		const ImageView _buffer;
		/**
		* Converts a 2D array of luminance data to 1 bit (true means black).
		*
		* @return The 2D array of bits for the image, nullptr on error.
		*/
		virtual std::shared_ptr<const BitMatrix> getBlackMatrix() const = 0;
		BitMatrix binarize(const uint8_t threshold) const;
	public:
		BinaryBitmap(const ImageView& buffer);
		virtual ~BinaryBitmap();
		int width() const { return _buffer.width(); }
		int height() const { return _buffer.height(); }
		/**
		* Converts one row of luminance data to a vector of ints denoting the widths of the bars and spaces.
		*/
		virtual bool getPatternRow(int row, int rotation, PatternRow& res) const = 0;
		const BitMatrix* getBitMatrix() const;
		void invert();
		bool inverted() const { return _inverted; }
		void close();
		bool closed() const { return _closed; }
	};
} // ZXing
//#include "Reader.h"
namespace ZXing {
	class BinaryBitmap;
	class DecodeHints;

	class Reader {
	protected:
		const DecodeHints& _hints;
	public:
		const bool supportsInversion;

		explicit Reader(const DecodeHints& hints, bool supportsInversion = false);
		explicit Reader(DecodeHints&& hints) = delete;
		virtual ~Reader() = default;
		virtual Result decode(const BinaryBitmap& image) const = 0;
		// WARNING: this API is experimental and may change/disappear
		virtual Results decode(const BinaryBitmap & image, [[maybe_unused]] int maxSymbols) const;
	};
} // ZXing
//#include "Matrix.h"
namespace ZXing {
	template <class T> class Matrix {
	public:
		using value_t = T;
	private:
		int _width = 0;
		int _height = 0;
		std::vector<value_t> _data;
		// Nothing wrong to support it, just to make it explicit, instead of by mistake.
		// Use copy() below.
		Matrix(const Matrix &) = default;
		Matrix & operator=(const Matrix &) = delete;
	public:
		Matrix() = default;
	#if defined(__llvm__) || (defined(__GNUC__) && (__GNUC__ > 7))
		__attribute__((no_sanitize("signed-integer-overflow")))
	#endif
		Matrix(int width, int height, value_t val = {}) : _width(width), _height(height), _data(_width * _height, val) 
		{
			if(width != 0 && Size(_data) / width != height)
				throw std::invalid_argument("invalid size: width * height is too big");
		}
		Matrix(Matrix&&) noexcept = default;
		Matrix& operator=(Matrix&&) noexcept = default;
		Matrix copy() const { return *this; }
		int height() const { return _height; }
		int width() const { return _width; }
		int size() const { return Size(_data); }
		value_t& operator()(int x, int y)
		{
			assert(x >= 0 && x < _width && y >= 0 && y < _height);
			return _data[y * _width + x];
		}
		const T& operator()(int x, int y) const
		{
			assert(x >= 0 && x < _width && y >= 0 && y < _height);
			return _data[y * _width + x];
		}
		const value_t& get(int x, int y) const { return operator()(x, y); }
		value_t& set(int x, int y, value_t value) { return operator()(x, y) = value; }
		const value_t& get(PointI p) const { return operator()(p.x, p.y); }
		value_t& set(PointI p, value_t value) { return operator()(p.x, p.y) = value; }
		const value_t* data() const { return _data.data(); }
		const value_t* begin() const { return _data.data(); }
		const value_t* end() const { return _data.data() + _width * _height; }
		void clear(value_t value = {}) { std::fill(_data.begin(), _data.end(), value); }
	};
} // ZXing
//#include "Range.h"
namespace ZXing {
	template <typename Iterator> struct StrideIter {
		Iterator pos;
		int stride;

		using iterator_category = std::random_access_iterator_tag;
		using difference_type   = typename std::iterator_traits<Iterator>::difference_type;
		using value_type        = typename std::iterator_traits<Iterator>::value_type;
		using pointer           = Iterator;
		using reference         = typename std::iterator_traits<Iterator>::reference;

		auto operator*() const { return *pos; }
		auto operator[](int i) const { return *(pos + i * stride); }
		StrideIter<Iterator>& operator++() { return pos += stride, *this; }
		StrideIter<Iterator> operator++(int) { auto temp = *this; ++*this; return temp; }
		bool operator!=(const StrideIter<Iterator>& rhs) const { return pos != rhs.pos; }
		StrideIter<Iterator> operator+(int i) const { return {pos + i * stride, stride}; }
		StrideIter<Iterator> operator-(int i) const { return {pos - i * stride, stride}; }
		int operator-(const StrideIter<Iterator>& rhs) const { return narrow_cast<int>((pos - rhs.pos) / stride); }
	};
	template <typename Iterator> StrideIter(const Iterator&, int) -> StrideIter<Iterator>;
	template <typename Iterator> struct Range {
		Iterator _begin, _end;
		Range(Iterator b, Iterator e) : _begin(b), _end(e) {}
		template <typename C> Range(const C& c) : _begin(std::begin(c)), _end(std::end(c)) {}
		Iterator begin() const noexcept { return _begin; }
		Iterator end() const noexcept { return _end; }
		explicit operator bool() const { return begin() < end(); }
		int size() const { return narrow_cast<int>(end() - begin()); }
	};

	template <typename C> Range(const C&) -> Range<typename C::const_iterator>;
} // namespace ZXing
//#include "BitArray.h"
namespace ZXing {
	class ByteArray;
	/**
	 * A simple, fast array of bits.
	 */
	class BitArray {
		std::vector<uint8_t> _bits;
		friend class BitMatrix;
		// Nothing wrong to support it, just to make it explicit, instead of by mistake.
		// Use copy() below.
		BitArray(const BitArray &) = default;
		BitArray& operator=(const BitArray &) = delete;
	public:
		using Iterator = std::vector<uint8_t>::const_iterator;
		BitArray() = default;
		explicit BitArray(int size) : _bits(size, 0) 
		{
		}
		BitArray(BitArray&& other) noexcept = default;
		BitArray& operator=(BitArray&& other) noexcept = default;
		BitArray copy() const { return *this; }
		int size() const noexcept { return Size(_bits); }
		int sizeInBytes() const noexcept { return (size() + 7) / 8; }
		bool get(int i) const { return _bits.at(i) != 0; }
		void set(int i, bool val) { _bits.at(i) = val; }
		// If you know exactly how may bits you are going to iterate
		// and that you access bit in sequence, iterator is faster than get().
		// However, be extremely careful since there is no check whatsoever.
		// (Performance is the reason for the iterator to exist in the first place.)
		Iterator iterAt(int i) const noexcept { return {_bits.cbegin() + i}; }
		Iterator begin() const noexcept { return _bits.cbegin(); }
		Iterator end() const noexcept { return _bits.cend(); }
		/**
		 * Appends the least-significant bits, from value, in order from most-significant to
		 * least-significant. For example, appending 6 bits from 0x000001E will append the bits
		 * 0, 1, 1, 1, 1, 0 in that order.
		 *
		 * @param value {@code int} containing bits to append
		 * @param numBits bits from value to append
		 */
		void appendBits(int value, int numBits)
		{
			for(; numBits; --numBits)
				_bits.push_back((value >> (numBits-1)) & 1);
		}
		void appendBit(bool bit) { _bits.push_back(bit); }
		void appendBitArray(const BitArray& other) { _bits.insert(_bits.end(), other.begin(), other.end()); }
		/**
		 * Reverses all bits in the array.
		 */
		void reverse() { std::reverse(_bits.begin(), _bits.end()); }
		void bitwiseXOR(const BitArray& other);
		/**
		 * @param bitOffset first bit to extract
		 * @param numBytes how many bytes to extract (-1 == until the end, padded with '0')
		 * @return Bytes are written most-significant bit first.
		 */
		ByteArray toBytes(int bitOffset = 0, int numBytes = -1) const;
		using Range = ZXing::Range<Iterator>;
		Range range() const { return {begin(), end()}; }
		friend bool operator==(const BitArray& a, const BitArray& b) { return a._bits == b._bits; }
	};

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T> > > T& AppendBit(T& val, bool bit)
	{
		return (val <<= 1) |= static_cast<T>(bit);
	}
	template <typename ARRAY, typename = std::enable_if_t<std::is_integral_v<typename ARRAY::value_type> > > int ToInt(const ARRAY& a)
	{
		assert(Reduce(a) <= 32);
		int pattern = 0;
		for(int i = 0; i < Size(a); i++)
			pattern = (pattern << a[i]) | ~(0xffffffff << a[i]) * (~i & 1);
		return pattern;
	}
	template <typename T = int, typename = std::enable_if_t<std::is_integral_v<T> > > T ToInt(const BitArray& bits, int pos = 0, int count = 8 * sizeof(T))
	{
		assert(0 <= count && count <= 8 * (int)sizeof(T));
		assert(0 <= pos && pos + count <= bits.size());
		count = smin(count, bits.size());
		int res = 0;
		auto it = bits.iterAt(pos);
		for(int i = 0; i < count; ++i, ++it)
			AppendBit(res, *it);
		return res;
	}
	template <typename T = int, typename = std::enable_if_t<std::is_integral_v<T> > > std::vector<T> ToInts(const BitArray& bits, int wordSize, int totalWords, int offset = 0)
	{
		assert(totalWords >= bits.size() / wordSize);
		assert(wordSize <= 8 * (int)sizeof(T));
		std::vector<T> res(totalWords, 0);
		for(int i = offset; i < bits.size(); i += wordSize)
			res[(i - offset) / wordSize] = ToInt(bits, i, wordSize);
		return res;
	}

	class BitArrayView {
		const BitArray & bits;
		BitArray::Iterator cur;
	public:
		BitArrayView(const BitArray& bits);
		BitArrayView & skipBits(int n);
		int peakBits(int n) const;
		int readBits(int n);
		int size() const { return narrow_cast<int>(bits.end() - cur); }
		explicit operator bool() const { return size(); }
	};
} // ZXing
//#include "BitMatrix.h"
namespace ZXing {
	class BitArray;
	class ByteMatrix;
	/**
	 * @brief A simple, fast 2D array of bits.
	 */
	class BitMatrix {
		int _width = 0;
		int _height = 0;
		using data_t = uint8_t;

		std::vector<data_t> _bits;
		// There is nothing wrong to support this but disable to make it explicit since we may copy something very big here.
		// Use copy() below.
		BitMatrix(const BitMatrix&) = default;
		BitMatrix& operator=(const BitMatrix&) = delete;

		const data_t & get(int i) const
		{
	#if 1
			return _bits.at(i);
	#else
			return _bits[i];
	#endif
		}
		data_t & get(int i) { return const_cast<data_t&>(static_cast<const BitMatrix*>(this)->get(i)); }
		bool getTopLeftOnBit(int &left, int& top) const;
		bool getBottomRightOnBit(int &right, int& bottom) const;
	public:
		static constexpr data_t SET_V = 0xff; // allows playing with SIMD binarization
		static constexpr data_t UNSET_V = 0;
		static_assert(bool(SET_V) && !bool(UNSET_V), "SET_V needs to evaluate to true, UNSET_V to false, see iterator usage");
		BitMatrix() = default;
	#if defined(__llvm__) || (defined(__GNUC__) && (__GNUC__ > 7))
		__attribute__((no_sanitize("signed-integer-overflow")))
	#endif
		BitMatrix(int width, int height) : _width(width), _height(height), _bits(width * height, UNSET_V)
		{
			if(width != 0 && Size(_bits) / width != height)
				throw std::invalid_argument("invalid size: width * height is too big");
		}
		explicit BitMatrix(int dimension) : BitMatrix(dimension, dimension) 
		{
		} // Construct a square matrix.
		BitMatrix(BitMatrix&& other) noexcept = default;
		BitMatrix& operator=(BitMatrix&& other) noexcept = default;
		BitMatrix copy() const { return *this; }
		Range<data_t*> row(int y) { return {_bits.data() + y * _width, _bits.data() + (y + 1) * _width}; }
		Range<const data_t*> row(int y) const { return {_bits.data() + y * _width, _bits.data() + (y + 1) * _width}; }
		Range<StrideIter<const data_t*> > col(int x) const
		{
			return {{_bits.data() + x + (_height - 1) * _width, -_width}, {_bits.data() + x - _width, -_width}};
		}
		bool get(int x, int y) const { return get(y * _width + x); }
		void set(int x, int y, bool val = true) { get(y * _width + x) = val * SET_V; }
		/**
		 * <p>Flips the given bit.</p>
		 *
		 * @param x The horizontal component (i.e. which column)
		 * @param y The vertical component (i.e. which row)
		 */
		void flip(int x, int y)
		{
			auto & v = get(y * _width + x);
			v = !v;
		}
		void flipAll()
		{
			for(auto & i : _bits)
				i = !i * SET_V;
		}
		/**
		 * <p>Sets a square region of the bit matrix to true.</p>
		 *
		 * @param left The horizontal position to begin at (inclusive)
		 * @param top The vertical position to begin at (inclusive)
		 * @param width The width of the region
		 * @param height The height of the region
		 */
		void setRegion(int left, int top, int width, int height);
		void rotate90();
		void rotate180();
		void mirror();
		/**
		 * Find the rectangle that contains all non-white pixels. Useful for detection of 'pure' barcodes.
		 *
		 * @return True iff this rectangle is at least minWidth x minHeight pixels big
		 */
		bool findBoundingBox(int &left, int& top, int& width, int& height, int minSize = 1) const;
		int width() const { return _width; }
		int height() const { return _height; }
		bool empty() const { return _bits.empty(); }
		friend bool operator==(const BitMatrix& a, const BitMatrix& b)
		{
			return a._width == b._width && a._height == b._height && a._bits == b._bits;
		}
		template <typename T> bool isIn(PointT<T> p, int b = 0) const noexcept
		{
			return b <= p.x && p.x < width() - b && b <= p.y && p.y < height() - b;
		}
		bool get(PointI p) const { return get(p.x, p.y); }
		bool get(PointF p) const { return get(PointI(p)); }
		void set(PointI p, bool v = true) { set(p.x, p.y, v); }
		void set(PointF p, bool v = true) { set(PointI(p), v); }
	};

	void GetPatternRow(const BitMatrix& matrix, int r, std::vector<uint16_t>& pr, bool transpose);

	/**
	 * @brief Inflate scales a BitMatrix up and adds a quiet Zone plus padding
	 * @param matrix input to be expanded
	 * @param width new width in bits (pixel)
	 * @param height new height in bits (pixel)
	 * @param quietZone size of quiet zone to add in modules
	 * @return expanded BitMatrix, maybe move(input) if size did not change
	 */
	BitMatrix Inflate(BitMatrix&& input, int width, int height, int quietZone);

	/**
	 * @brief Deflate (crop + subsample) a bit matrix
	 * @param matrix
	 * @param width new width
	 * @param height new height
	 * @param top cropping starts at top row
	 * @param left cropping starts at left col
	 * @param subSampling typically the module size
	 * @return deflated input
	 */
	BitMatrix Deflate(const BitMatrix& matrix, int width, int height, float top, float left, float subSampling);

	template <typename T> BitMatrix ToBitMatrix(const Matrix<T>& in, T trueValue = {true})
	{
		BitMatrix out(in.width(), in.height());
		for(int y = 0; y < in.height(); ++y)
			for(int x = 0; x < in.width(); ++x)
				if(in.get(x, y) == trueValue)
					out.set(x, y);
		return out;
	}
	template <typename T> Matrix<T> ToMatrix(const BitMatrix & in, T black = 0, T white = ~0)
	{
		Matrix<T> res(in.width(), in.height());
		for(int y = 0; y < in.height(); ++y)
			for(int x = 0; x < in.width(); ++x)
				res.set(x, y, in.get(x, y) ? black : white);
		return res;
	}
} // ZXing
//#include "BitMatrixCursor.h"
namespace ZXing {
	enum class Direction { LEFT = -1, RIGHT = 1 };

	inline Direction opposite(Direction dir) noexcept { return dir == Direction::LEFT ? Direction::RIGHT : Direction::LEFT; }

	/**
	 * @brief The BitMatrixCursor represents a current position inside an image and current direction it can advance
	 *towards.
	 *
	 * The current position and direction is a PointT<T>. So depending on the type it can be used to traverse the image
	 * in a Bresenham style (PointF) or in a discrete way (step only horizontal/vertical/diagonal (PointI)).
	 */
	template <typename POINT> class BitMatrixCursor {
		using this_t = BitMatrixCursor<POINT>;
	public:
		const BitMatrix* img;
		POINT p; // current position
		POINT d; // current direction
		BitMatrixCursor(const BitMatrix& image, POINT p, POINT d) : img(&image), p(p) 
		{
			setDirection(d);
		}
		class Value {
			enum { INVALID = -1, WHITE = 0, BLACK = 1 };
			int v = INVALID;
	public:
			Value() = default;
			Value(bool isBlack) : v(isBlack) 
			{
			}
			bool isValid() const noexcept { return v != INVALID; }
			bool isWhite() const noexcept { return v == WHITE; }
			bool isBlack() const noexcept { return v == BLACK; }
			operator bool() const noexcept { return isValid(); }
			bool operator==(Value o) const { return v == o.v; }
			bool operator!=(Value o) const { return v != o.v; }
		};

		template <typename T> Value testAt(PointT<T> p) const { return img->isIn(p) ? Value{img->get(p)} : Value{}; }
		bool blackAt(POINT pos) const noexcept { return testAt(pos).isBlack(); }
		bool whiteAt(POINT pos) const noexcept { return testAt(pos).isWhite(); }
		bool isIn(POINT p) const noexcept { return img->isIn(p); }
		bool isIn() const noexcept { return isIn(p); }
		bool isBlack() const noexcept { return blackAt(p); }
		bool isWhite() const noexcept { return whiteAt(p); }
		POINT front() const noexcept { return d; }
		POINT back() const noexcept { return {-d.x, -d.y}; }
		POINT left() const noexcept { return {d.y, -d.x}; }
		POINT right() const noexcept { return {-d.y, d.x}; }
		POINT direction(Direction dir) const noexcept { return static_cast<int>(dir) * right(); }
		void turnBack() noexcept { d = back(); }
		void turnLeft() noexcept { d = left(); }
		void turnRight() noexcept { d = right(); }
		void turn(Direction dir) noexcept { d = direction(dir); }
		Value edgeAt(POINT d) const noexcept 
		{ 
			Value v = testAt(p);
			return testAt(p + d) != v ? v : Value();
		}
		Value edgeAtFront() const noexcept { return edgeAt(front()); }
		Value edgeAtBack() const noexcept { return edgeAt(back()); }
		Value edgeAtLeft() const noexcept { return edgeAt(left()); }
		Value edgeAtRight() const noexcept { return edgeAt(right()); }
		Value edgeAt(Direction dir) const noexcept { return edgeAt(direction(dir)); }
		this_t& setDirection(PointF dir) { return d = bresenhamDirection(dir), *this; }
		this_t& setDirection(PointI dir) { return d = dir, *this; }
		bool step(typename POINT::value_t s = 1)
		{
			p += s * d;
			return isIn(p);
		}
		this_t movedBy(POINT o) const noexcept { return {*img, p + o, d}; }
		this_t turnedBack() const noexcept { return {*img, p, back()}; }
		/**
		 * @brief stepToEdge advances cursor to one step behind the next (or n-th) edge.
		 * @param nth number of edges to pass
		 * @param range max number of steps to take
		 * @param backup whether or not to backup one step so we land in front of the edge
		 * @return number of steps taken or 0 if moved outside of range/image
		 */
		int stepToEdge(int nth = 1, int range = 0, bool backup = false)
		{
			int steps = 0;
			auto lv = testAt(p);
			while(nth && (!range || steps < range) && lv.isValid()) {
				++steps;
				auto v = testAt(p + steps * d);
				if(lv != v) {
					lv = v;
					--nth;
				}
			}
			if(backup)
				--steps;
			p += steps * d;
			return steps * (nth == 0);
		}

		bool stepAlongEdge(Direction dir, bool skipCorner = false)
		{
			if(!edgeAt(dir))
				turn(dir);
			else if(edgeAtFront()) {
				turn(opposite(dir));
				if(edgeAtFront()) {
					turn(opposite(dir));
					if(edgeAtFront())
						return false;
				}
			}
			bool ret = step();
			if(ret && skipCorner && !edgeAt(dir)) {
				turn(dir);
				ret = step();
			}

			return ret;
		}

		int countEdges(int range)
		{
			int res = 0;
			while(int steps = range ? stepToEdge(1, range) : 0) {
				range -= steps;
				++res;
			}
			return res;
		}
		template <typename ARRAY> ARRAY readPattern(int range = 0)
		{
			ARRAY res = {};
			for(auto & i : res) {
				i = stepToEdge(1, range);
				if(!i)
					return res;
				if(range)
					range -= i;
			}
			return res;
		}
		template <typename ARRAY> ARRAY readPatternFromBlack(int maxWhitePrefix, int range = 0)
		{
			if(maxWhitePrefix && isWhite() && !stepToEdge(1, maxWhitePrefix))
				return {};
			return readPattern<ARRAY>(range);
		}
	};

	using BitMatrixCursorF = BitMatrixCursor<PointF>;
	using BitMatrixCursorI = BitMatrixCursor<PointI>;

	class FastEdgeToEdgeCounter {
		const uint8_t * p = nullptr;
		int stride = 0;
		int stepsToBorder = 0;
	public:
		FastEdgeToEdgeCounter(const BitMatrixCursorI& cur);
		int stepToNextEdge(int range);
	};
} // ZXing
//#include "Pattern.h"
namespace ZXing {
	using PatternType = uint16_t;
	template <int N> using Pattern = std::array<PatternType, N>;
	using PatternRow = std::vector<PatternType>;

	class PatternView {
		using Iterator = PatternRow::const_pointer;
		Iterator _data = nullptr;
		int _size = 0;
		Iterator _base = nullptr;
		Iterator _end = nullptr;
	public:
		using value_type = PatternRow::value_type;
		PatternView() = default;
		// A PatternRow always starts with the width of whitespace in front of the first black bar.
		// The first element of the PatternView is the first bar.
		PatternView(const PatternRow& bars) : _data(bars.data() + 1), _size(Size(bars) - 1), _base(bars.data()), _end(bars.data() + bars.size())
		{
		}
		PatternView(Iterator data, int size, Iterator base, Iterator end) : _data(data), _size(size), _base(base), _end(end) 
		{
		}
		template <size_t N> PatternView(const Pattern<N>& row) : _data(row.data()), _size(N)
		{
		}
		Iterator data() const { return _data; }
		Iterator begin() const { return _data; }
		Iterator end() const { return _data + _size; }
		value_type operator[](int i) const
		{
	//		assert(i < _count);
			return _data[i];
		}
		int sum(int n = 0) const { return std::accumulate(_data, _data + (n == 0 ? _size : n), 0); }
		int size() const { return _size; }
		// index is the number of bars and spaces from the first bar to the current position
		int index() const { return narrow_cast<int>(_data - _base) - 1; }
		int pixelsInFront() const { return std::accumulate(_base, _data, 0); }
		int pixelsTillEnd() const { return std::accumulate(_base, _data + _size, 0) - 1; }
		bool isAtFirstBar() const { return _data == _base + 1; }
		bool isAtLastBar() const { return _data + _size == _end - 1; }
		bool isValid(int n) const { return _data && _data >= _base && _data + n <= _end; }
		bool isValid() const { return isValid(size()); }
		template <bool acceptIfAtFirstBar = false> bool hasQuietZoneBefore(float scale) const
		{
			return (acceptIfAtFirstBar && isAtFirstBar()) || _data[-1] >= sum() * scale;
		}
		template <bool acceptIfAtLastBar = true> bool hasQuietZoneAfter(float scale) const
		{
			return (acceptIfAtLastBar && isAtLastBar()) || _data[_size] >= sum() * scale;
		}
		PatternView subView(int offset, int size = 0) const
		{
	//		if(std::abs(size) > count())
	//			printf("%d > %d\n", std::abs(size), _count);
	//		assert(std::abs(size) <= count());
			if(size == 0)
				size = _size - offset;
			else if(size < 0)
				size = _size - offset + size;
			return {begin() + offset, smax(size, 0), _base, _end};
		}
		bool shift(int n) { return _data && ((_data += n) + _size <= _end); }
		bool skipPair() { return shift(2); }
		bool skipSymbol() { return shift(_size); }
		bool skipSingle(int maxWidth) { return shift(1) && _data[-1] <= maxWidth; }
		void extend() { _size = smax(0, narrow_cast<int>(_end - _data)); }
	};
	/**
	 * @brief The BarAndSpace struct is a simple 2 element data structure to hold information about bar(s) and space(s).
	 *
	 * The operator[](int) can be used in combination with a PatternView
	 */
	template <typename T> struct BarAndSpace {
		using value_type = T;
		T bar = {}, space = {};
		// even index -> bar, odd index -> space
		constexpr T& operator[](int i) noexcept { return reinterpret_cast<T*>(this)[i & 1]; }
		constexpr T operator[](int i) const noexcept { return reinterpret_cast<const T*>(this)[i & 1]; }
		bool isValid() const { return bar != T{} && space != T{}; }
	};

	using BarAndSpaceI = BarAndSpace<PatternType>;

	template <int LEN, typename T, typename RT = T> constexpr auto BarAndSpaceSum(const T* view) noexcept
	{
		BarAndSpace<RT> res;
		for(int i = 0; i < LEN; ++i)
			res[i] += view[i];
		return res;
	}

	/**
	 * @brief FixedPattern describes a compile-time constant (start/stop) pattern.
	 *
	 * @param N  number of bars/spaces
	 * @param SUM  sum over all N elements (size of pattern in modules)
	 * @param IS_SPARCE  whether or not the pattern contains '0's denoting 'wide' bars/spaces
	 */
	template <int N, int SUM, bool IS_SPARCE = false> struct FixedPattern {
		using value_type = PatternRow::value_type;
		value_type _data[N];
		constexpr value_type operator[](int i) const noexcept { return _data[i]; }
		constexpr const value_type* data() const noexcept { return _data; }
		constexpr int size() const noexcept { return N; }
		constexpr BarAndSpace<value_type> sums() const noexcept { return BarAndSpaceSum<N>(_data); }
	};

	template <int N, int SUM> using FixedSparcePattern = FixedPattern<N, SUM, true>;

	template <bool E2E = false, int LEN, int SUM> float IsPattern(const PatternView& view, const FixedPattern<LEN, SUM, false>& pattern, int spaceInPixel = 0,
		float minQuietZone = 0, float moduleSizeRef = 0)
	{
		if constexpr(E2E) {
			using float_t = double;
			auto widths = BarAndSpaceSum<LEN, PatternView::value_type, float_t>(view.data());
			auto sums = pattern.sums();
			BarAndSpace<float_t> modSize = {widths[0] / sums[0], widths[1] / sums[1]};
			auto [m, M] = std::minmax(modSize[0], modSize[1]);
			if(M > 4 * m)  // make sure module sizes of bars and spaces are not too far away from each other
				return 0;
			if(minQuietZone && spaceInPixel < minQuietZone * modSize.space)
				return 0;
			const BarAndSpace<float_t> thr = {modSize[0] * .75 + .5, modSize[1] / (2 + (LEN < 6)) + .5};
			for(int x = 0; x < LEN; ++x)
				if(std::abs(view[x] - pattern[x] * modSize[x]) > thr[x])
					return 0;
			const float_t moduleSize = (modSize[0] + modSize[1]) / 2;
			return moduleSize;
		}
		int width = view.sum(LEN);
		if(SUM > LEN && width < SUM)
			return 0;
		const float moduleSize = (float)width / SUM;
		if(minQuietZone && spaceInPixel < minQuietZone * moduleSize - 1)
			return 0;
		if(!moduleSizeRef)
			moduleSizeRef = moduleSize;
		// the offset of 0.5 is to make the code less sensitive to quantization errors for small (near 1) module sizes.
		// TODO: review once we have upsampling in the binarizer in place.
		const float threshold = moduleSizeRef * (0.5f + E2E * 0.25f) + 0.5f;
		for(int x = 0; x < LEN; ++x)
			if(std::abs(view[x] - pattern[x] * moduleSizeRef) > threshold)
				return 0;
		return moduleSize;
	}

	template <bool RELAXED_THRESHOLD = false, int N, int SUM>
	float IsPattern(const PatternView& view, const FixedPattern<N, SUM, true>& pattern, int spaceInPixel = 0,
		float minQuietZone = 0, float moduleSizeRef = 0)
	{
		// pattern contains the indices with the bars/spaces that need to be equally wide
		int width = 0;
		for(int x = 0; x < SUM; ++x)
			width += view[pattern[x]];
		const float moduleSize = (float)width / SUM;
		if(minQuietZone && spaceInPixel < minQuietZone * moduleSize - 1)
			return 0;
		if(!moduleSizeRef)
			moduleSizeRef = moduleSize;
		// the offset of 0.5 is to make the code less sensitive to quantization errors for small (near 1) module sizes.
		// TODO: review once we have upsampling in the binarizer in place.
		const float threshold = moduleSizeRef * (0.5f + RELAXED_THRESHOLD * 0.25f) + 0.5f;
		for(int x = 0; x < SUM; ++x)
			if(std::abs(view[pattern[x]] - moduleSizeRef) > threshold)
				return 0;
		return moduleSize;
	}
	template <int N, int SUM, bool IS_SPARCE> bool IsRightGuard(const PatternView& view, const FixedPattern<N, SUM, IS_SPARCE>& pattern, float minQuietZone,
		float moduleSizeRef = 0.f)
	{
		int spaceInPixel = view.isAtLastBar() ? std::numeric_limits<int>::max() : *view.end();
		return IsPattern(view, pattern, spaceInPixel, minQuietZone, moduleSizeRef) != 0;
	}
	template <int LEN, typename Pred> PatternView FindLeftGuard(const PatternView& view, int minSize, Pred isGuard)
	{
		if(view.size() < minSize)
			return {};
		auto window = view.subView(0, LEN);
		if(window.isAtFirstBar() && isGuard(window, std::numeric_limits<int>::max()))
			return window;
		for(auto end = view.end() - minSize; window.data() < end; window.skipPair())
			if(isGuard(window, window[-1]))
				return window;

		return {};
	}
	template <int LEN, int SUM, bool IS_SPARCE> PatternView FindLeftGuard(const PatternView& view, int minSize, const FixedPattern<LEN, SUM, IS_SPARCE>& pattern,
		float minQuietZone)
	{
		return FindLeftGuard<LEN>(view, smax(minSize, LEN),
			   [&pattern, minQuietZone](const PatternView& window, int spaceInPixel) {
				return IsPattern(window, pattern, spaceInPixel, minQuietZone);
			});
	}
	template <int LEN, int SUM> std::array<int, LEN - 2> NormalizedE2EPattern(const PatternView& view)
	{
		float moduleSize = static_cast<float>(view.sum(LEN)) / SUM;
		std::array<int, LEN - 2> e2e;
		for(int i = 0; i < LEN - 2; i++) {
			float v = (view[i] + view[i + 1]) / moduleSize;
			e2e[i] = int(v + .5f);
		}
		return e2e;
	}
	template <int LEN, int SUM> std::array<int, LEN> NormalizedPattern(const PatternView& view)
	{
		float moduleSize = static_cast<float>(view.sum(LEN)) / SUM;
		int err = SUM;
		std::array<int, LEN> is;
		std::array<float, LEN> rs;
		for(int i = 0; i < LEN; i++) {
			float v = view[i] / moduleSize;
			is[i] = int(v + .5f);
			rs[i] = v - is[i];
			err -= is[i];
		}
		if(std::abs(err) > 1)
			return {};
		if(err) {
			auto mi = err > 0 ? std::max_element(std::begin(rs), std::end(rs)) - std::begin(rs)
							  : std::min_element(std::begin(rs), std::end(rs)) - std::begin(rs);
			is[mi] += err;
			rs[mi] -= err;
		}
		return is;
	}
	template <typename I> void GetPatternRow(Range<I> b_row, PatternRow& p_row)
	{
		// TODO: if reactivating the bit-packed array (!ZX_FAST_BIT_STORAGE) should be of interest then the following code could be
		// considerably speed up by using a specialized variant along the lines of the old BitArray::getNextSetTo() function that
		// was removed between 1.4 and 2.0.
	#if 0
		p_row.reserve(64);
		p_row.clear();
		auto lastPos = b_row.begin();
		if(*lastPos)
			p_row.push_back(0); // first value is number of white pixels, here 0
		for(auto p = b_row.begin() + 1; p < b_row.end(); ++p)
			if(bool(*p) != bool(*lastPos))
				p_row.push_back(p - std::exchange(lastPos, p));
		p_row.push_back(b_row.end() - lastPos);
		if(*lastPos)
			p_row.push_back(0); // last value is number of white pixels, here 0
	#else
		p_row.resize(b_row.size() + 2);
		std::fill(p_row.begin(), p_row.end(), 0);

		auto bitPos = b_row.begin();
		const auto bitPosEnd = b_row.end();
		auto intPos = p_row.data();

		if(*bitPos)
			intPos++; // first value is number of white pixels, here 0

		// The following code as been observed to cause a speedup of up to 30% on large images on an AVX cpu
		// and on an a Google Pixel 3 Android phone. Your mileage may vary.
		if constexpr(std::is_pointer_v<I> && sizeof(I) == 8 && sizeof(std::remove_pointer_t<I>) == 1) {
			using simd_t = uint64_t;
			while(bitPos < bitPosEnd - sizeof(simd_t)) {
				auto asSimd0 = BitHacks::LoadU<simd_t>(bitPos);
				auto asSimd1 = BitHacks::LoadU<simd_t>(bitPos + 1);
				auto z = asSimd0 ^ asSimd1;
				if(z) {
	//#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	#if defined(SL_LITTLEENDIAN)
					int step = /*BitHacks::NumberOfTrailingZeros*/SBits::Ctz(z) / 8 + 1;
	#else
					int step = /*BitHacks::NumberOfLeadingZeros*/SBits::Clz(z) / 8 + 1;
	#endif
					(*intPos++) += step;
					bitPos += step;
				}
				else {
					(*intPos) += sizeof(simd_t);
					bitPos += sizeof(simd_t);
				}
			}
		}
		while(++bitPos != bitPosEnd) {
			++(*intPos);
			intPos += bitPos[0] != bitPos[-1];
		}
		++(*intPos);
		if(bitPos[-1])
			intPos++;
		p_row.resize(intPos - p_row.data() + 1);
	#endif
	}
} // ZXing
//#include "ConcentricFinder.h"
namespace ZXing {
	template <typename T, size_t N> static float CenterFromEnd(const std::array<T, N>& pattern, float end)
	{
		if(N == 5) {
			float a = pattern[4] + pattern[3] + pattern[2] / 2.f;
			float b = pattern[4] + (pattern[3] + pattern[2] + pattern[1]) / 2.f;
			float c = (pattern[4] + pattern[3] + pattern[2] + pattern[1] + pattern[0]) / 2.f;
			return end - (2 * a + b + c) / 4;
		}
		else if(N == 3) {
			float a = pattern[2] + pattern[1] / 2.f;
			float b = (pattern[2] + pattern[1] + pattern[0]) / 2.f;
			return end - (2 * a + b) / 3;
		}
		else {   // aztec
			auto a = std::accumulate(pattern.begin() + (N/2 + 1), pattern.end(), pattern[N/2] / 2.f);
			return end - a;
		}
	}
	template <int N, typename Cursor> std::optional<Pattern<N> > ReadSymmetricPattern(Cursor& cur, int range)
	{
		static_assert(N % 2 == 1);
		assert(range > 0);
		Pattern<N> res = {};
		auto constexpr s_2 = Size(res)/2;
		auto cuo = cur.turnedBack();

		auto next = [&](auto & cur, int i) {
				auto v = cur.stepToEdge(1, range);
				res[s_2 + i] += v;
				if(range)
					range -= v;
				return v;
			};

		for(int i = 0; i <= s_2; ++i) {
			if(!next(cur, i) || !next(cuo, -i))
				return {};
		}
		res[s_2]--; // the starting pixel has been counted twice, fix this
		return res;
	}
	template <bool RELAXED_THRESHOLD = false, typename PATTERN> int CheckSymmetricPattern(BitMatrixCursorI& cur, PATTERN pattern, int range, bool updatePosition)
	{
		FastEdgeToEdgeCounter curFwd(cur), curBwd(cur.turnedBack());
		int centerFwd = curFwd.stepToNextEdge(range);
		if(!centerFwd)
			return 0;
		int centerBwd = curBwd.stepToNextEdge(range);
		if(!centerBwd)
			return 0;
		assert(range > 0);
		Pattern<pattern.size()> res = {};
		auto constexpr s_2 = Size(res)/2;
		res[s_2] = centerFwd + centerBwd - 1; // -1 because the starting pixel is counted twice
		range -= res[s_2];
		auto next = [&](auto & cur, int i) {
				auto v = cur.stepToNextEdge(range);
				res[s_2 + i] = v;
				range -= v;
				return v;
			};
		for(int i = 1; i <= s_2; ++i) {
			if(!next(curFwd, i) || !next(curBwd, -i))
				return 0;
		}
		if(!IsPattern<RELAXED_THRESHOLD>(res, pattern))
			return 0;
		if(updatePosition)
			cur.step(res[s_2] / 2 - (centerBwd - 1));
		return Reduce(res);
	}

	std::optional<PointF> CenterOfRing(const BitMatrix& image, PointI center, int range, int nth, bool requireCircle = true);
	std::optional<PointF> FinetuneConcentricPatternCenter(const BitMatrix& image, PointF center, int range, int finderPatternSize);
	std::optional<QuadrilateralF> FindConcentricPatternCorners(const BitMatrix& image, PointF center, int range, int ringIndex);

	struct ConcentricPattern : public PointF {
		int size = 0;
	};

	template <bool E2E = false, typename PATTERN> std::optional<ConcentricPattern> LocateConcentricPattern(const BitMatrix& image, PATTERN pattern, PointF center, int range)
	{
		auto cur = BitMatrixCursor(image, PointI(center), {});
		int minSpread = image.width(), maxSpread = 0;
		// TODO: setting maxError to 1 can subtantially help with detecting symbols with low print quality resulting in
		// damaged
		// finder patterns, but it sutantially increases the runtime (approx. 20% slower for the falsepositive images).
		int maxError = 0;
		for(auto d : {PointI{0, 1}, {1, 0}}) {
			int spread = CheckSymmetricPattern<E2E>(cur.setDirection(d), pattern, range, true);
			if(spread)
				UpdateMinMax(minSpread, maxSpread, spread);
			else if(--maxError < 0)
				return {};
		}

	#if 1
		for(auto d : {PointI{1, 1}, {1, -1}}) {
			int spread = CheckSymmetricPattern<true>(cur.setDirection(d), pattern, range * 2, false);
			if(spread)
				UpdateMinMax(minSpread, maxSpread, spread);
			else if(--maxError < 0)
				return {};
		}
	#endif
		if(maxSpread > 5 * minSpread)
			return {};
		auto newCenter = FinetuneConcentricPatternCenter(image, PointF(cur.p), range, pattern.size());
		if(!newCenter)
			return {};
		return ConcentricPattern{*newCenter, (maxSpread + minSpread) / 2};
	}
} // ZXing
//#include "LogMatrix.h"
namespace ZXing {
	#ifdef PRINT_DEBUG
		class LogMatrix {
			using LogBuffer = Matrix<uint8_t>;
			LogBuffer _log;
			const BitMatrix* _image = nullptr;
			int _scale = 1;
		public:
			void init(const BitMatrix* image, int scale = 1)
			{
				_image = image;
				_scale = scale;
				_log = LogBuffer(_image->width() * _scale, _image->height() * _scale);
			}
			void write(const char* fn)
			{
				assert(_image);
				FILE* f = fopen(fn, "wb");

				// Write PPM header, P5 == grey, P6 == rgb
				fprintf(f, "P6\n%d %d\n255\n", _log.width(), _log.height());

				// Write pixels
				for(int y = 0; y < _log.height(); ++y)
					for(int x = 0; x < _log.width(); ++x) {
						uchar pix[3];
						uchar &r = pix[0], &g = pix[1], &b = pix[2];
						r = g = b = _image->get(x / _scale, y / _scale) ? 0 : 255;
						if(_scale > 1 && x % _scale == _scale / 2 && y % _scale == _scale / 2)
							r = g = b = r ? 230 : 50;
						switch(_log.get(x, y)) {
							case 1: r = g = b = _scale > 1 ? 128 : (r ? 230 : 50); break;
							case 2: r = b = 50, g = 220; break;
							case 3: g = r = 100, b = 250; break;
							case 4: g = b = 100, r = 250; break;
						}
						fwrite(&pix, 3, 1, f);
					}
				fclose(f);
			}
			template <typename T> void operator()(const PointT<T>& p, int color = 1)
			{
				if(_image && _image->isIn(p))
					_log.set(static_cast<int>(p.x * _scale), static_cast<int>(p.y * _scale), color);
			}
			void operator()(const PointT<int>& p, int color) { operator()(centered(p), color); }
			template <typename T> void operator()(const std::vector<PointT<T> >& points, int color = 2)
			{
				for(auto p : points)
					operator()(p, color);
			}
		};

		extern LogMatrix log;

		class LogMatrixWriter {
			LogMatrix &log;
			std::string fn;
		public:
			LogMatrixWriter(LogMatrix& log, const BitMatrix& image, int scale, std::string fn) : log(log), fn(fn)
			{
				log.init(&image, scale);
			}
			~LogMatrixWriter() 
			{
				log.write(fn.c_str());
			}
		};
	#else
		template <typename T> void log(PointT<T>, int = 0) 
		{
		}
	#endif
} // namespace ZXing
//#include "RegressionLine.h"
namespace ZXing {
	class RegressionLine {
	protected:
		std::vector<PointF> _points;
		PointF _directionInward;
		PointF::value_t a = NAN, b = NAN, c = NAN;

		friend PointF intersect(const RegressionLine& l1, const RegressionLine& l2);
		template <typename T> bool evaluate(const PointT<T>* begin, const PointT<T>* end)
		{
			auto mean = std::accumulate(begin, end, PointF()) / std::distance(begin, end);
			PointF::value_t sumXX = 0, sumYY = 0, sumXY = 0;
			for(auto p = begin; p != end; ++p) {
				auto d = *p - mean;
				sumXX += d.x * d.x;
				sumYY += d.y * d.y;
				sumXY += d.x * d.y;
			}
			if(sumYY >= sumXX) {
				auto l = std::sqrt(sumYY * sumYY + sumXY * sumXY);
				a = +sumYY / l;
				b = -sumXY / l;
			}
			else {
				auto l = std::sqrt(sumXX * sumXX + sumXY * sumXY);
				a = +sumXY / l;
				b = -sumXX / l;
			}
			if(dot(_directionInward, normal()) < 0) {
				a = -a;
				b = -b;
			}
			c = dot(normal(), mean); // (a*mean.x + b*mean.y);
			return dot(_directionInward, normal()) > 0.5f; // angle between original and new direction is at most 60 degree
		}
		template <typename T> bool evaluate(const std::vector<PointT<T> >& points) { return evaluate(&points.front(), &points.back() + 1); }
		template <typename T> static auto distance(PointT<T> a, PointT<T> b) { return ZXing::distance(a, b); }
	public:
		RegressionLine() 
		{
			_points.reserve(16);
		} // arbitrary but plausible start size (tiny performance improvement)
		template <typename T> RegressionLine(PointT<T> a, PointT<T> b)
		{
			evaluate(std::vector{a, b});
		}
		template <typename T> RegressionLine(const PointT<T>* b, const PointT<T>* e)
		{
			evaluate(b, e);
		}
		const auto & points() const { return _points; }
		int length() const { return _points.size() >= 2 ? int(distance(_points.front(), _points.back())) : 0; }
		bool isValid() const { return !std::isnan(a); }
		PointF normal() const { return isValid() ? PointF(a, b) : _directionInward; }
		auto signedDistance(PointF p) const { return dot(normal(), p) - c; }
		template <typename T> auto distance(PointT<T> p) const { return std::abs(signedDistance(PointF(p))); }
		PointF project(PointF p) const { return p - signedDistance(p) * normal(); }
		PointF centroid() const { return std::accumulate(_points.begin(), _points.end(), PointF()) / _points.size(); }
		void reset()
		{
			_points.clear();
			_directionInward = {};
			a = b = c = NAN;
		}
		void add(PointF p) 
		{
			assert(_directionInward != PointF());
			_points.push_back(p);
			if(_points.size() == 1)
				c = dot(normal(), p);
		}
		void pop_back() 
		{
			_points.pop_back();
		}
		void pop_front()
		{
			std::rotate(_points.begin(), _points.begin() + 1, _points.end());
			_points.pop_back();
		}
		void setDirectionInward(PointF d) 
		{
			_directionInward = normalized(d);
		}
		bool evaluate(double maxSignedDist = -1, bool updatePoints = false)
		{
			bool ret = evaluate(_points);
			if(maxSignedDist > 0) {
				auto points = _points;
				while(true) {
					auto old_points_size = points.size();
					// remove points that are further 'inside' than maxSignedDist or further 'outside' than
					// 2 x maxSignedDist
					auto end = std::remove_if(points.begin(), points.end(), [this, maxSignedDist](auto p) {
							auto sd = this->signedDistance(p);
							return sd > maxSignedDist || sd < -2 * maxSignedDist;
						});
					points.erase(end, points.end());
					// if we threw away too many points, something is off with the line to begin with
					if(points.size() < old_points_size / 2 || points.size() < 2)
						return false;
					if(old_points_size == points.size())
						break;
	#ifdef PRINT_DEBUG
					printf("removed %zu points -> %zu remaining\n", old_points_size - points.size(), points.size());
	#endif
					ret = evaluate(points);
				}

				if(updatePoints)
					_points = std::move(points);
			}
			return ret;
		}
		bool isHighRes() const
		{
			PointF min = _points.front(), max = _points.front();
			for(auto p : _points) {
				UpdateMinMax(min.x, max.x, p.x);
				UpdateMinMax(min.y, max.y, p.y);
			}
			auto diff  = max - min;
			auto len   = maxAbsComponent(diff);
			auto steps = smin(std::abs(diff.x), std::abs(diff.y));
			// due to aliasing we get bad extrapolations if the line is short and too close to vertical/horizontal
			return steps > 2 || len > 50;
		}
	};

	inline PointF intersect(const RegressionLine& l1, const RegressionLine& l2)
	{
		assert(l1.isValid() && l2.isValid());
		auto d = l1.a * l2.b - l1.b * l2.a;
		auto x = (l1.c * l2.b - l1.b * l2.c) / d;
		auto y = (l1.a * l2.c - l1.c * l2.a) / d;
		return {x, y};
	}
} // ZXing
//#include "ECI.h"
namespace ZXing {
	enum class ECI : int {
		Unknown    = -1,
		Cp437      = 2, // obsolete
		ISO8859_1  = 3,
		ISO8859_2  = 4,
		ISO8859_3  = 5,
		ISO8859_4  = 6,
		ISO8859_5  = 7,
		ISO8859_6  = 8,
		ISO8859_7  = 9,
		ISO8859_8  = 10,
		ISO8859_9  = 11,
		ISO8859_10 = 12,
		ISO8859_11 = 13,
		ISO8859_13 = 15,
		ISO8859_14 = 16,
		ISO8859_15 = 17,
		ISO8859_16 = 18,
		Shift_JIS  = 20,
		Cp1250     = 21,
		Cp1251     = 22,
		Cp1252     = 23,
		Cp1256     = 24,
		UTF16BE    = 25,
		UTF8       = 26,
		ASCII      = 27,
		Big5       = 28,
		GB2312     = 29,
		EUC_KR     = 30,
		GB18030    = 32,
		UTF16LE    = 33,
		UTF32BE    = 34,
		UTF32LE    = 35,
		ISO646_Inv = 170,
		Binary     = 899
	};

	inline constexpr int ToInt(ECI eci) { return static_cast<int>(eci); }
	inline constexpr bool IsText(ECI eci) { return ToInt(eci) >= 0 && ToInt(eci) <= 170; }
	inline constexpr bool CanProcess(ECI eci)
	{
		// see https://github.com/zxing-cpp/zxing-cpp/commit/d8587545434d533c4e568181e1c12ef04a8e42d9#r74864359
		return ToInt(eci) <= 899;
	}
	/**
	 * @brief ToString converts the numerical ECI value to a 7 character string as used in the ECI protocol
	 * @return e.g. "\000020"
	 */
	std::string ToString(ECI eci);
	CharacterSet ToCharacterSet(ECI eci);
	ECI ToECI(CharacterSet cs);
} // namespace ZXing
//#include "HRI.h"
namespace ZXing {
	std::string HRIFromGS1(std::string_view gs1);
	std::string HRIFromISO15434(std::string_view str);
} // namespace ZXing
//#include "TextDecoder.h"
namespace ZXing {
	class TextDecoder {
	public:
		static CharacterSet DefaultEncoding();
		static CharacterSet GuessEncoding(const uint8_t* bytes, size_t length, CharacterSet fallback = DefaultEncoding());
		// If `sjisASCII` set then for Shift_JIS maps ASCII directly (straight-thru), i.e. does not map ASCII backslash & tilde
		// to Yen sign & overline resp. (JIS X 0201 Roman)
		static void Append(std::string& str, const uint8_t* bytes, size_t length, CharacterSet charset, bool sjisASCII = true);
		static void Append(std::wstring& str, const uint8_t* bytes, size_t length, CharacterSet charset);
		static void AppendLatin1(std::wstring& str, const std::string& latin1) 
		{
			auto ptr = (const uint8_t*)latin1.data();
			str.append(ptr, ptr + latin1.length());
		}
		static std::wstring FromLatin1(const std::string& latin1) 
		{
			auto ptr = (const uint8_t*)latin1.data();
			return std::wstring(ptr, ptr + latin1.length());
		}
	};
} // ZXing
//#include "Utf.h"
namespace ZXing {
	std::string ToUtf8(std::wstring_view str);
	std::wstring FromUtf8(std::string_view utf8);
	#if __cplusplus > 201703L
		std::wstring FromUtf8(std::u8string_view utf8);
	#endif

	std::wstring EscapeNonGraphical(std::wstring_view str);
	std::string EscapeNonGraphical(std::string_view utf8);
} // namespace ZXing
//#include "TextUtfEncoding.h"
namespace ZXing::TextUtfEncoding {
	// The following functions are not required anymore after Result::text() now returns utf8 natively and the encoders accept utf8 as well.
	[[deprecated]] std::string ToUtf8(std::wstring_view str);
	[[deprecated]] std::string ToUtf8(std::wstring_view str, const bool angleEscape);
	[[deprecated]] std::wstring FromUtf8(std::string_view utf8);
} // namespace ZXing::TextUtfEncoding
//#include "TextEncoder.h"
namespace ZXing {
	class TextEncoder {
		static void GetBytes(const std::string& str, CharacterSet charset, std::string& bytes);
		static void GetBytes(const std::wstring& str, CharacterSet charset, std::string& bytes);
	public:
		static std::string FromUnicode(const std::string& str, CharacterSet charset);
		static std::string FromUnicode(const std::wstring& str, CharacterSet charset);
	};
} // ZXing
//#include "ResultPoint.h"
namespace ZXing {
	/**
	* <p>Encapsulates a point of interest in an image containing a barcode. Typically, this
	* would be the location of a finder pattern or the corner of the barcode, for example.</p>
	*
	* @author Sean Owen
	*/
	class ResultPoint : public PointF {
	public:
		ResultPoint() = default;
		ResultPoint(float x, float y) : PointF(x, y) {}
		ResultPoint(int x, int y) : PointF(x, y) {}
		template <typename T> ResultPoint(PointT<T> p) : PointF(p) {}
		float x() const { return static_cast<float>(PointF::x); }
		float y() const { return static_cast<float>(PointF::y); }
		void set(float x, float y) { *this = PointF(x, y); }
		static float Distance(int aX, int aY, int bX, int bY);
	};
} // ZXing
//#include "GenericGFPoly.h"
namespace ZXing {
	class GenericGF;
	/**
	 * <p>Represents a polynomial whose coefficients are elements of a GF.
	 * Instances of this class are immutable.</p>
	 *
	 * <p>Much credit is due to William Rucklidge since portions of this code are an indirect
	 * port of his C++ Reed-Solomon implementation.</p>
	 *
	 * @author Sean Owen
	 */
	class GenericGFPoly {
		struct Coefficients : public std::vector<int> {
			void reserve(size_t s);
			void resize(size_t s);
			void resize(size_t s, int i);
		};
	public:
		// Build a invalid object, so that this can be used in container or return by reference,
		// any access to invalid object is undefined behavior.
		GenericGFPoly() = default;
		/**
		 * @param field the {@link GenericGF} instance representing the field to use
		 * to perform computations
		 * @param coefficients coefficients as ints representing elements of GF(size), arranged
		 * from most significant (highest-power term) coefficient to least significant
		 */
		GenericGFPoly(const GenericGF& field, std::vector<int>&& coefficients) : _field(&field)
		{
			assert(!coefficients.empty());
			_coefficients.swap(coefficients); // _coefficients = coefficients
			normalize();
		}
		GenericGFPoly(const GenericGF& field, const std::vector<int>& coefficients) : GenericGFPoly(field, std::vector<int>(coefficients)) 
		{
		}
		GenericGFPoly& operator=(GenericGFPoly&& other) noexcept = default;
		GenericGFPoly(GenericGFPoly&& other) noexcept = default;
		GenericGFPoly& operator=(const GenericGFPoly& other) 
		{
			assert(_field == other._field);
			_coefficients.reserve(other._coefficients.size());
			_coefficients = other._coefficients;
			return *this;
		}
		GenericGFPoly(const GenericGFPoly& other) 
		{
			_field = other._field;
			*this = other;
		}
		GenericGFPoly& setField(const GenericGF& field)
		{
			_field = &field;
			return *this;
		}
		const GenericGF& field() const noexcept { return *_field; }
		const auto & coefficients() const noexcept { return _coefficients; }
		/**
		 * @return degree of this polynomial
		 */
		int degree() const { return Size(_coefficients) - 1; }
		/**
		 * @return true iff this polynomial is the monomial "0"
		 */
		bool isZero() const { return _coefficients[0] == 0; }
		int leadingCoefficient() const noexcept { return _coefficients.front(); }
		int constant() const noexcept { return _coefficients.back(); }

		/**
		 * @brief set to the monomial representing coefficient * x^degree
		 */
		GenericGFPoly& setMonomial(int coefficient, int degree = 0)
		{
			assert(degree >= 0 && (coefficient != 0 || degree == 0));
			_coefficients.resize(degree + 1);
			std::fill(_coefficients.begin(), _coefficients.end(), 0);
			_coefficients.front() = coefficient;
			return *this;
		}
		/**
		 * @return evaluation of this polynomial at a given point
		 */
		int evaluateAt(int a) const;
		GenericGFPoly& addOrSubtract(GenericGFPoly& other);
		GenericGFPoly& multiply(const GenericGFPoly& other);
		GenericGFPoly& multiplyByMonomial(int coefficient, int degree = 0);
		GenericGFPoly& divide(const GenericGFPoly& other, GenericGFPoly& quotient);
		friend void swap(GenericGFPoly& a, GenericGFPoly& b)
		{
			std::swap(a._field, b._field);
			std::swap(a._coefficients, b._coefficients);
		}
	private:
		void normalize();

		const GenericGF* _field = nullptr;
		Coefficients _coefficients, _cache; // _cache is used for malloc caching
	};
} // ZXing
//#include "GenericGF.h"
namespace ZXing {
	/**
	 * <p>This class contains utility methods for performing mathematical operations over
	 * the Galois Fields. Operations use a given primitive polynomial in calculations.</p>
	 *
	 * <p>Throughout this package, elements of the GF are represented as an {@code int}
	 * for convenience and speed (but at the cost of memory).
	 * </p>
	 *
	 * @author Sean Owen
	 * @author David Olivier
	 */
	class GenericGF {
		const int _size;
		int _generatorBase;
		std::vector<short> _expTable;
		std::vector<short> _logTable;
		/**
		 * Create a representation of GF(size) using the given primitive polynomial.
		 *
		 * @param primitive irreducible polynomial whose coefficients are represented by
		 *  the bits of an int, where the least-significant bit represents the constant
		 *  coefficient
		 * @param size the size of the field (m = log2(size) is called the word size of the encoding)
		 * @param b the factor b in the generator polynomial can be 0- or 1-based
		 *  (g(x) = (x+a^b)(x+a^(b+1))...(x+a^(b+2t-1))).
		 *  In most cases it should be 1, but for QR code it is 0.
		 */
		GenericGF(int primitive, int size, int b);
	public:
		static const GenericGF& AztecData12();
		static const GenericGF& AztecData10();
		static const GenericGF& AztecData6();
		static const GenericGF& AztecParam();
		static const GenericGF& QRCodeField256();
		static const GenericGF& DataMatrixField256();
		static const GenericGF& AztecData8();
		static const GenericGF& MaxiCodeField64();

		// note: replaced addOrSubstract calls with '^' / '^='. everyone trying to understand this code needs to look
		// into
		// Galois Fields with characteristic 2 and will then understand that XOR is addition/subtraction. And those
		// operators are way more readable than a noisy member function name

		/**
		 * @return 2 to the power of a in GF(size)
		 */
		int exp(int a) const { return _expTable.at(a); }
		/**
		 * @return base 2 log of a in GF(size)
		 */
		int log(int a) const 
		{
			if(a == 0) {
				throw std::invalid_argument("a == 0");
			}
			return _logTable.at(a);
		}
		/**
		 * @return multiplicative inverse of a
		 */
		int inverse(int a) const { return _expTable[_size - log(a) - 1]; }
		/**
		 * @return product of a and b in GF(size)
		 */
		int multiply(int a, int b) const noexcept 
		{
			if(a == 0 || b == 0)
				return 0;
	#ifdef ZX_REED_SOLOMON_USE_MORE_MEMORY_FOR_SPEED
			return _expTable[_logTable[a] + _logTable[b]];
	#else
			auto fast_mod = [](const int input, const int ceil) {
					// avoid using the '%' modulo operator => ReedSolomon computation is more than twice as fast
					// see also https://stackoverflow.com/a/33333636/2088798
					return input < ceil ? input : input - ceil;
				};
			return _expTable[fast_mod(_logTable[a] + _logTable[b], _size - 1)];
	#endif
		}
		int size() const noexcept { return _size; }
		int generatorBase() const noexcept { return _generatorBase; }
	};
} // namespace ZXing
//#include "ReadBarcode.h"
namespace ZXing {
	/**
	 * Read barcode from an ImageView
	 *
	 * @param buffer  view of the image data including layout and format
	 * @param hints  optional DecodeHints to parameterize / speed up decoding
	 * @return #Result structure
	 */
	Result ReadBarcode(const ImageView& buffer, const DecodeHints& hints = {});
	/**
	 * Read barcodes from an ImageView
	 *
	 * @param buffer  view of the image data including layout and format
	 * @param hints  optional DecodeHints to parameterize / speed up decoding
	 * @return #Results list of results found, may be empty
	 */
	Results ReadBarcodes(const ImageView& buffer, const DecodeHints& hints = {});
} // ZXing
//#include "GlobalHistogramBinarizer.h"
namespace ZXing {
	/**
	* This Binarizer implementation uses the old ZXing global histogram approach. It is suitable
	* for low-end mobile devices which don't have enough CPU or memory to use a local thresholding
	* algorithm. However, because it picks a global black point, it cannot handle difficult shadows
	* and gradients.
	*
	* Faster mobile devices and all desktop applications should probably use HybridBinarizer instead.
	*
	* @author dswitkin@google.com (Daniel Switkin)
	* @author Sean Owen
	*/
	class GlobalHistogramBinarizer : public BinaryBitmap {
	public:
		explicit GlobalHistogramBinarizer(const ImageView& buffer);
		~GlobalHistogramBinarizer() override;
		bool getPatternRow(int row, int rotation, PatternRow &res) const override;
		std::shared_ptr<const BitMatrix> getBlackMatrix() const override;
	};
} // ZXing
//#include "HybridBinarizer.h"
namespace ZXing {
	/**
	* This class implements a local thresholding algorithm, which while slower than the
	* GlobalHistogramBinarizer, is fairly efficient for what it does. It is designed for
	* high frequency images of barcodes with black data on white backgrounds. For this application,
	* it does a much better job than a global blackpoint with severe shadows and gradients.
	* However it tends to produce artifacts on lower frequency images and is therefore not
	* a good general purpose binarizer for uses outside ZXing.
	*
	* This class extends GlobalHistogramBinarizer, using the older histogram approach for 1D readers,
	* and the newer local approach for 2D readers. 1D decoding using a per-row histogram is already
	* inherently local, and only fails for horizontal gradients. We can revisit that problem later,
	* but for now it was not a win to use local blocks for 1D.
	*
	* This Binarizer is the default for the unit tests and the recommended class for library users.
	*
	* @author dswitkin@google.com (Daniel Switkin)
	*/
	class HybridBinarizer : public GlobalHistogramBinarizer {
	public:
		explicit HybridBinarizer(const ImageView& iv);
		~HybridBinarizer() override;
		bool getPatternRow(int row, int rotation, PatternRow &res) const override;
		std::shared_ptr<const BitMatrix> getBlackMatrix() const override;
	};
} // ZXing
//#include "ThresholdBinarizer.h"
namespace ZXing {
	class ThresholdBinarizer : public BinaryBitmap {
		const uint8_t _threshold = 0;
	public:
		ThresholdBinarizer(const ImageView& buffer, uint8_t threshold = 128);
		bool getPatternRow(int row, int rotation, PatternRow& res) const override;
		std::shared_ptr<const BitMatrix> getBlackMatrix() const override;
	};
} // ZXing
//#include "GTIN.h"
namespace ZXing {
	class Result;
	namespace GTIN {
		template <typename T> T ComputeCheckDigit(const std::basic_string<T>& digits, bool skipTail = false)
		{
			int sum = 0, N = Size(digits) - skipTail;
			for(int i = N - 1; i >= 0; i -= 2)
				sum += digits[i] - '0';
			sum *= 3;
			for(int i = N - 2; i >= 0; i -= 2)
				sum += digits[i] - '0';
			return ToDigit<T>((10 - (sum % 10)) % 10);
		}
		template <typename T> bool IsCheckDigitValid(const std::basic_string<T>& s)
		{
			return ComputeCheckDigit(s, true) == s.back();
		}
		/**
		 * Evaluate the prefix of the GTIN to estimate the country of origin. See
		 * <a href="https://www.gs1.org/standards/id-keys/company-prefix">
		 * https://www.gs1.org/standards/id-keys/company-prefix</a> and
		 * <a href="https://en.wikipedia.org/wiki/List_of_GS1_country_codes">
		 * https://en.wikipedia.org/wiki/List_of_GS1_country_codes</a>.
		 *
		 * `format` required for EAN-8 (UPC-E assumed if not given)
		 */
		std::string LookupCountryIdentifier(const std::string& GTIN, const BarcodeFormat format = BarcodeFormat::None);
		std::string EanAddOn(const Result& result);
		std::string IssueNr(const std::string& ean2AddOn);
		std::string Price(const std::string& ean5AddOn);
	} // namespace GTIN
} // namespace ZXing
//#include "DecoderResult.h"
namespace ZXing {
	class CustomData;

	class DecoderResult {
		Content _content;
		std::string _ecLevel;
		int _lineCount = 0;
		int _versionNumber = 0;
		StructuredAppendInfo _structuredAppend;
		bool _isMirrored = false;
		bool _readerInit = false;
		Error _error;
		std::shared_ptr<CustomData> _extra;
		DecoderResult(const DecoderResult &) = delete;
		DecoderResult& operator=(const DecoderResult &) = delete;
	public:
		DecoderResult() = default;
		DecoderResult(Error error) : _error(std::move(error)) {}
		DecoderResult(Content&& bytes) : _content(std::move(bytes)) {}
		DecoderResult(DecoderResult&&) noexcept = default;
		DecoderResult& operator=(DecoderResult&&) noexcept = default;
		bool isValid(bool includeErrors = false) const
		{
			return includeErrors || (_content.symbology.code != 0 && !_error);
		}
		const Content& content() const & { return _content; }
		Content&& content() && { return std::move(_content); }

		// to keep the unit tests happy for now:
		std::wstring text() const { return _content.utfW(); }
		std::string symbologyIdentifier() const { return _content.symbology.toString(false); }

		// Simple macro to set up getter/setter methods that save lots of boilerplate.
		// It sets up a standard 'const & () const', 2 setters for setting lvalues via
		// copy and 2 for setting rvalues via move. They are provided each to work
		// either on lvalues (normal 'void (...)') or on rvalues (returning '*this' as
		// rvalue). The latter can be used to optionally initialize a temporary in a
		// return statement, e.g.
		//    return DecoderResult(bytes, text).setEcLevel(level);
	#define ZX_PROPERTY(TYPE, GETTER, SETTER) \
		const TYPE& GETTER() const & { return _##GETTER; } \
		TYPE&& GETTER() && { return std::move(_##GETTER); } \
		void SETTER(const TYPE& v) & { _##GETTER = v; } \
		void SETTER(TYPE&& v) & { _##GETTER = std::move(v); } \
		DecoderResult&& SETTER(const TYPE& v) && { _##GETTER = v; return std::move(*this); } \
		DecoderResult&& SETTER(TYPE&& v) && { _##GETTER = std::move(v); return std::move(*this); }

		ZX_PROPERTY(std::string, ecLevel, setEcLevel)
		ZX_PROPERTY(int, lineCount, setLineCount)
		ZX_PROPERTY(int, versionNumber, setVersionNumber)
		ZX_PROPERTY(StructuredAppendInfo, structuredAppend, setStructuredAppend)
		ZX_PROPERTY(Error, error, setError)
		ZX_PROPERTY(bool, isMirrored, setIsMirrored)
		ZX_PROPERTY(bool, readerInit, setReaderInit)
		ZX_PROPERTY(std::shared_ptr<CustomData>, extra, setExtra)

	#undef ZX_PROPERTY
	};
} // ZXing
//#include "MultiFormatWriter.h"
namespace ZXing {
	class BitMatrix;
	/**
	* This class is here just for convenience as it offers single-point service
	* to generate barcodes for all supported formats. As a result, this class
	* offer very limited customization compared to what are available in each
	* individual encoder.
	*/
	class MultiFormatWriter {
	public:
		explicit MultiFormatWriter(BarcodeFormat format) : _format(format) {}
		/**
		* Used for Aztec, PDF417, and QRCode only.
		*/
		MultiFormatWriter & setEncoding(CharacterSet encoding) 
		{
			_encoding = encoding;
			return *this;
		}
		/**
		* Used for Aztec, PDF417, and QRCode only, [0-8].
		*/
		MultiFormatWriter & setEccLevel(int level) 
		{
			_eccLevel = level;
			return *this;
		}
		/**
		* Used for all formats, sets the minimum number of quiet zone pixels.
		*/
		MultiFormatWriter & setMargin(int margin) 
		{
			_margin = margin;
			return *this;
		}
		BitMatrix encode(const std::wstring& contents, int width, int height) const;
		BitMatrix encode(const std::string& contents, int width, int height) const;
	private:
		BarcodeFormat _format;
		CharacterSet _encoding = CharacterSet::Unknown;
		int _margin = -1;
		int _eccLevel = -1;
	};
} // ZXing
//#include "BitSource.h"
namespace ZXing {
	class ByteArray;
	/**
	* <p>This provides an easy abstraction to read bits at a time from a sequence of bytes, where the
	* number of bits read is not often a multiple of 8.</p>
	*
	* <p>This class is thread-safe but not reentrant -- unless the caller modifies the bytes array
	* it passed in, in which case all bets are off.</p>
	*
	* @author Sean Owen
	*/
	class BitSource {
		const ByteArray& _bytes;
		int _byteOffset = 0;
		int _bitOffset = 0;
	public:
		/**
		* @param bytes bytes from which this will read bits. Bits will be read from the first byte first.
		* Bits are read within a byte from most-significant to least-significant bit.
		* IMPORTANT: Bit source DOES NOT copy data byte, thus make sure that the bytes outlive the bit source object.
		*/
		explicit BitSource(const ByteArray& bytes) : _bytes(bytes) {}
		BitSource(BitSource &) = delete;
		BitSource& operator=(const BitSource &) = delete;
		/**
		* @return index of next bit in current byte which would be read by the next call to {@link #readBits(int)}.
		*/
		int bitOffset() const { return _bitOffset; }
		/**
		* @return index of next byte in input byte array which would be read by the next call to {@link #readBits(int)}.
		*/
		int byteOffset() const { return _byteOffset; }
		/**
		* @param numBits number of bits to read
		* @return int representing the bits read. The bits will appear as the least-significant bits of the int
		*/
		int readBits(int numBits);
		/**
		* @param numBits number of bits to peak
		* @return int representing the bits peaked.  The bits will appear as the least-significant
		*         bits of the int
		*/
		int peakBits(int numBits) const;
		/**
		* @return number of bits that can be read successfully
		*/
		int available() const;
	};
} // ZXing
//#include "BitMatrixIO.h"
namespace ZXing {
	std::string ToString(const BitMatrix& matrix, char one = 'X', char zero = ' ', bool addSpace = true, bool printAsCString = false);
	std::string ToSVG(const BitMatrix& matrix);
	BitMatrix ParseBitMatrix(const std::string& str, char one = 'X', bool expectSpace = true);
	void SaveAsPBM(const BitMatrix& matrix, const std::string filename, int quietZone = 0);
} // ZXing
//#include "DetectorResult.h"
namespace ZXing {
	/**
	* Encapsulates the result of detecting a barcode in an image. This includes the raw
	* matrix of black/white pixels corresponding to the barcode and the position of the code
	* in the input image.
	*/
	class DetectorResult {
		BitMatrix _bits;
		QuadrilateralI _position;

		DetectorResult(const DetectorResult&) = delete;
		DetectorResult& operator=(const DetectorResult&) = delete;
	public:
		DetectorResult() = default;
		DetectorResult(DetectorResult&&) noexcept = default;
		DetectorResult& operator=(DetectorResult&&) noexcept = default;
		DetectorResult(BitMatrix&& bits, QuadrilateralI&& position) : _bits(std::move(bits)), _position(std::move(position)) {}
		const BitMatrix& bits() const & { return _bits; }
		BitMatrix&& bits() && { return std::move(_bits); }
		const QuadrilateralI& position() const & { return _position; }
		QuadrilateralI&& position() && { return std::move(_position); }
		bool isValid() const { return !_bits.empty(); }
	};
} // ZXing
//#include "PerspectiveTransform.h"
namespace ZXing {
	/**
	 * <p>This class implements a perspective transform in two dimensions. Given four source and four
	 * destination points, it will compute the transformation implied between them. The code is based
	 * directly upon section 3.4.2 of George Wolberg's "Digital Image Warping"; see pages 54-56.</p>
	 */
	class PerspectiveTransform {
		using value_t = PointF::value_t;
		value_t a11, a12, a13, a21, a22, a23, a31, a32, a33 = NAN;

		PerspectiveTransform(value_t a11, value_t a21, value_t a31, value_t a12, value_t a22, value_t a32, value_t a13, value_t a23, value_t a33)
			: a11(a11), a12(a12), a13(a13), a21(a21), a22(a22), a23(a23), a31(a31), a32(a32), a33(a33)
		{
		}
		PerspectiveTransform inverse() const;
		PerspectiveTransform times(const PerspectiveTransform& other) const;
		static PerspectiveTransform UnitSquareTo(const QuadrilateralF& q);
	public:
		PerspectiveTransform() = default;
		PerspectiveTransform(const QuadrilateralF& src, const QuadrilateralF& dst);
		/// Project from the destination space (grid of modules) into the image space (bit matrix)
		PointF operator()(PointF p) const;
		bool isValid() const { return !std::isnan(a33); }
	};
} // ZXing
//#include "GridSampler.h"
namespace ZXing {
	/**
	* Samples an image for a rectangular matrix of bits of the given dimension. The sampling
	* transformation is determined by the coordinates of 4 points, in the original and transformed
	* image space.
	*
	* The following figure is showing the layout a 'pixel'. The point (0,0) is the upper left corner
	* of the first pixel. (1,1) is its lower right corner.
	*
	*   0    1   ...   w
	* 0 #----#-- ... --#
	*   |    |   ...   |
	*   |    |   ...   |
	* 1 #----#   ... --#
	*   |    |   ...   |
	*
	*   |    |   ...   |
	* h #----#-- ... --#
	*
	* @param image image to sample
	* @param width width of {@link BitMatrix} to sample from image
	* @param height height of {@link BitMatrix} to sample from image
	* @param mod2Pix transforming a module (grid) coordinate into an image (pixel) coordinate
	* @return {@link DetectorResult} representing a grid of points sampled from the image within a region
	*   defined by the "src" parameters. Result is empty if transformation is invalid (out of bound access).
	*/
	DetectorResult SampleGrid(const BitMatrix& image, int width, int height, const PerspectiveTransform& mod2Pix);

	template <typename PointT = PointF> Quadrilateral<PointT> Rectangle(int x0, int x1, int y0, int y1, typename PointT::value_t o = 0.5)
	{
		return {PointT{x0 + o, y0 + o}, {x1 + o, y0 + o}, {x1 + o, y1 + o}, {x0 + o, y1 + o}};
	}

	class ROI {
	public:
		int x0, x1, y0, y1;
		PerspectiveTransform mod2Pix;
	};

	using ROIs = std::vector<ROI>;

	DetectorResult SampleGrid(const BitMatrix& image, int width, int height, const ROIs& rois);
} // ZXing
//#include "ReedSolomonEncoder.h"
namespace ZXing {
	// public only for testing purposes
	class ReedSolomonEncoder {
	public:
		explicit ReedSolomonEncoder(const GenericGF& field);
		void encode(std::vector<int>& message, int numECCodeWords);
	private:
		const GenericGF* _field;
		std::list<GenericGFPoly> _cachedGenerators;
		const GenericGFPoly& buildGenerator(int degree);
	};
	/**
	 * @brief ReedSolomonEncode replaces the last numECCodeWords code words in message with error correction code words
	 */
	inline void ReedSolomonEncode(const GenericGF& field, std::vector<int>& message, int numECCodeWords)
	{
		ReedSolomonEncoder(field).encode(message, numECCodeWords);
	}
} // namespace ZXing
//#include "ReedSolomonDecoder.h"
namespace ZXing {
	class GenericGF;

	/**
	* <p>Implements Reed-Solomon decoding, as the name implies.</p>
	*
	* <p>The algorithm will not be explained here, but the following references were helpful
	* in creating this implementation:</p>
	*
	* <ul>
	* <li>Bruce Maggs.
	* <a href="http://www.cs.cmu.edu/afs/cs.cmu.edu/project/pscico-guyb/realworld/www/rs_decode.ps">
	* "Decoding Reed-Solomon Codes"</a> (see discussion of Forney's Formula)</li>
	* <li>J.I. Hall. <a href="www.mth.msu.edu/~jhall/classes/codenotes/GRS.pdf">
	* "Chapter 5. Generalized Reed-Solomon Codes"</a>
	* (see discussion of Euclidean algorithm)</li>
	* </ul>
	*
	* <p>Much credit is due to William Rucklidge since portions of this code are an indirect
	* port of his C++ Reed-Solomon implementation.</p>
	*
	* @author Sean Owen
	* @author William Rucklidge
	* @author sanfordsquires
	*/

	/**
	 * @brief ReedSolomonDecode fixes errors in a message containing both data and parity codewords.
	 *
	 * @param message data and error-correction/parity codewords
	 * @param numECCodeWords number of error-correction code words
	 * @return true iff message errors could successfully be fixed (or there have not been any)
	 */
	bool ReedSolomonDecode(const GenericGF& field, std::vector<int>& message, int numECCodeWords);
} // ZXing
//#include "WhiteRectDetector.h"
namespace ZXing {
	class BitMatrix;
	class ResultPoint;
	/**
	 * <p>
	 * Detects a candidate barcode-like rectangular region within an image. It
	 * starts around the center of the image, increases the size of the candidate
	 * region until it finds a white rectangular region.
	 * </p>
	 *
	 * @param image barcode image to find a rectangle in
	 * @param initSize initial size of search area around center
	 * @param x x position of search center
	 * @param y y position of search center
	 * @return {@link ResultPoint}[] describing the corners of the rectangular
	 *         region. The first and last points are opposed on the diagonal, as
	 *         are the second and third. The first point will be the topmost
	 *         point and the last, the bottommost. The second point will be
	 *         leftmost and the third, the rightmost
	 * @return true iff white rect was found
	 */
	bool DetectWhiteRect(const BitMatrix& image, int initSize, int x, int y, ResultPoint& p0, ResultPoint& p1, ResultPoint& p2, ResultPoint& p3);
	bool DetectWhiteRect(const BitMatrix& image, ResultPoint& p0, ResultPoint& p1, ResultPoint& p2, ResultPoint& p3);
} // ZXing
//#include "ZXNullable.h"
namespace ZXing {
	template <typename T> class Nullable final {
		bool m_hasValue = false;
		T m_value;
	public:
		Nullable() = default;
		Nullable(const T &value) : m_hasValue(true), m_value(value) 
		{
		}
		Nullable(T &&value) noexcept : m_hasValue(true), m_value(std::move(value)) 
		{
		}
		Nullable(std::nullptr_t) 
		{
		}
		Nullable<T> & operator=(const T &value)
		{
			m_hasValue = true;
			m_value = value;
			return *this;
		}
		Nullable<T> & operator=(T &&value) noexcept
		{
			m_hasValue = true;
			m_value = std::move(value);
			return *this;
		}
		Nullable<T> & operator=(std::nullptr_t)
		{
			m_hasValue = false;
			m_value = T();
			return *this;
		}
		operator T() const 
		{
			if(!m_hasValue) {
				throw std::logic_error("Access empty value");
			}
			return m_value;
		}
		bool hasValue() const { return m_hasValue; }
		const T & value() const { return m_value; }
		T & value() { return m_value; }
		friend inline bool operator==(const Nullable &a, const Nullable &b) { return a.m_hasValue == b.m_hasValue && (!a.m_hasValue || a.m_value == b.m_value); }
		friend inline bool operator!=(const Nullable &a, const Nullable &b) { return !(a == b); }
		friend inline bool operator==(const Nullable &a, const T &v) { return a.m_hasValue && a.m_value == v; }
		friend inline bool operator!=(const Nullable &a, const T &v) { return !(a == v); }
		friend inline bool operator==(const T &v, const Nullable &a) { return a.m_hasValue && a.m_value == v; }
		friend inline bool operator!=(const T &v, const Nullable &a) { return !(v == a); }
		friend inline bool operator==(const Nullable &a, std::nullptr_t) { return !a.m_hasValue; }
		friend inline bool operator!=(const Nullable &a, std::nullptr_t) { return a.m_hasValue; }
		friend inline bool operator==(std::nullptr_t, const Nullable &a) { return !a.m_hasValue; }
		friend inline bool operator!=(std::nullptr_t, const Nullable &a) { return a.m_hasValue; }
		friend inline void swap(Nullable &a, Nullable &b) 
		{
			using std::swap;
			swap(a.m_value, b.m_value);
			swap(a.m_hasValue, b.m_hasValue);
		}
	};
} // ZXing
//#include "ZXBigInteger.h"
namespace ZXing {
	/**
	 * All credits on BigInteger below go to Matt McCutchen, as the code below is extracted/modified from his C++ Big
	 *Integer Library (https://mattmccutchen.net/bigint/)
	 */
	class BigInteger {
	public:
		using Block = size_t;

		// The number of bits in a block
		//static const uint N = 8 * sizeof(Block);

		// Constructs zero.
		BigInteger() = default;

		template <typename T> BigInteger(T x, typename std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> >* = nullptr) : mag(1, x) 
		{
		}
		template <typename T> BigInteger(T x, typename std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> >* = nullptr) : negative(x < 0), mag(1, std::abs(x)) 
		{
		}
		static bool TryParse(const std::string& str, BigInteger& result);
		static bool TryParse(const std::wstring& str, BigInteger& result);
		bool isZero() const { return mag.empty(); }
		std::string toString() const;
		int toInt() const;
		BigInteger& operator+=(BigInteger&& a) 
		{
			if(mag.empty())
				*this = std::move(a);
			else
				Add(*this, a, *this);
			return *this;
		}
		friend BigInteger operator+(const BigInteger& a, const BigInteger& b) 
		{
			BigInteger c;
			BigInteger::Add(a, b, c);
			return c;
		}
		friend BigInteger operator-(const BigInteger& a, const BigInteger& b) 
		{
			BigInteger c;
			BigInteger::Subtract(a, b, c);
			return c;
		}
		friend BigInteger operator*(const BigInteger& a, const BigInteger& b) 
		{
			BigInteger c;
			BigInteger::Multiply(a, b, c);
			return c;
		}
		static void Add(const BigInteger& a, const BigInteger &b, BigInteger& c);
		static void Subtract(const BigInteger& a, const BigInteger &b, BigInteger& c);
		static void Multiply(const BigInteger& a, const BigInteger &b, BigInteger& c);
		static void Divide(const BigInteger& a, const BigInteger &b, BigInteger& quotient, BigInteger& remainder);
	private:
		bool negative = false;
		std::vector<Block> mag;
	};
} // ZXing
//#include "ByteMatrix.h"
namespace ZXing {
	// TODO: If kept at all, this should be replaced by `using ByteMatrix = Matrix<uint8_t>;` to be consistent with ByteArray
	// This non-template class is kept for now to stay source-compatible with older versions of the library.
	class ByteMatrix : public Matrix<int8_t> {
	public:
		ByteMatrix() = default;
		ByteMatrix(int width, int height, int8_t val = 0) : Matrix<int8_t>(width, height, val) {}
		ByteMatrix(ByteMatrix&&) noexcept = default;
		ByteMatrix& operator=(ByteMatrix&&) noexcept = default;
	};
} // ZXing
//#include "Scope.h"
namespace ZXing {
	/**
	 * ScopeExit is a trivial helper meant to be later replaced by std::scope_exit from library fundamentals TS v3.
	 */
	template <typename EF> class ScopeExit {
		EF fn;
	public:
		ScopeExit(EF&& fn) : fn(std::move(fn)) 
		{
		}
		~ScopeExit() 
		{
			fn();
		}
	};
	template <class EF> ScopeExit(EF)->ScopeExit<EF>;
	/**
	 * The SCOPE_EXIT macro is eliminating the need to give the object a name.
	 * Example usage:
	 *   SCOPE_EXIT([]{ printf("exiting scope"); });
	 */
	#define SCOPE_EXIT_CAT2(x, y) x ## y
	#define SCOPE_EXIT_CAT(x, y) SCOPE_EXIT_CAT2(x, y)
	#define SCOPE_EXIT const auto SCOPE_EXIT_CAT(scopeExit_, __COUNTER__) = ScopeExit
} // ZXing
//#include "TritMatrix.h"
namespace ZXing {
	/**
	 * @brief Represent a tri-state value false/true/empty
	 */
	class Trit {
	public:
		enum value_t : uint8_t {false_v, true_v, empty_v} value = empty_v;
		Trit() = default;
		Trit(bool v) : value(static_cast<value_t>(v)) {}
		operator bool() const { return value == true_v; }
		bool isEmpty() const { return value == empty_v; }
	};
	using TritMatrix = Matrix<Trit>;
} // ZXing
//#include "CustomData.h"
namespace ZXing {
	class CustomData {
	public:
		virtual ~CustomData() = default;
	protected:
		CustomData() = default;
	};
} // ZXing
#ifdef __cpp_impl_coroutine
	#include <Generator.h>
	//#include <DetectorResult.h>
#endif

//#include "datamatrix/DMVersion.h"
namespace ZXing::DataMatrix {
	/**
	 * The Version object encapsulates attributes about a particular Data Matrix Symbol size.
	 */
	class Version {
	public:
		/**
		 * Encapsulates a set of error-correction blocks in one symbol version. Most versions will
		 * use blocks of differing sizes within one version, so, this encapsulates the parameters for
		 * each set of blocks. It also holds the number of error-correction codewords per block since it
		 * will be the same across all blocks within one version.
		 */
		struct ECBlocks {
			int codewordsPerBlock;
			/* Encapsulates the parameters for one error-correction block in one symbol version.
			 * This includes the number of data codewords, and the number of times a block with these
			 * parameters is used consecutively in the Data Matrix code version's format.
			 */
			struct {
				int count;
				int dataCodewords;
			} const blocks[2];
			int numBlocks() const { return blocks[0].count + blocks[1].count; }
			int totalDataCodewords() const
			{
				return blocks[0].count * (blocks[0].dataCodewords + codewordsPerBlock) + blocks[1].count * (blocks[1].dataCodewords + codewordsPerBlock);
			}
		};
		const int versionNumber;
		const int symbolHeight;
		const int symbolWidth;
		const int dataBlockHeight;
		const int dataBlockWidth;
		const ECBlocks ecBlocks;

		int totalCodewords() const { return ecBlocks.totalDataCodewords(); }
		int dataWidth() const { return (symbolWidth / dataBlockWidth) * dataBlockWidth; }
		int dataHeight() const { return (symbolHeight / dataBlockHeight) * dataBlockHeight; }
		bool isDMRE() const { return versionNumber >= 31 && versionNumber <= 48; }
	};
	/**
	 * @brief Looks up Version information based on symbol dimensions.
	 *
	 * @param height Number of rows in modules
	 * @param width Number of columns in modules
	 * @return Version for a Data Matrix Code of those dimensions, nullputr for invalid dimensions
	 */
	const Version * VersionForDimensions(int height, int width);
	template<typename MAT> const Version* VersionForDimensionsOf(const MAT& mat)
	{
		return VersionForDimensions(mat.height(), mat.width());
	}
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMReader.h"
namespace ZXing::DataMatrix {
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;
		Result decode(const BinaryBitmap& image) const override;
	#ifdef __cpp_impl_coroutine
		Results decode(const BinaryBitmap& image, int maxSymbols) const override;
	#endif
	};
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMBitLayout.h"
namespace ZXing {
	class BitMatrix;
	class ByteArray;

	namespace DataMatrix {
		class Version;

		BitMatrix BitMatrixFromCodewords(const ByteArray& codewords, int width, int height);
		ByteArray CodewordsFromBitMatrix(const BitMatrix& bits, const Version& version);
	} // namespace DataMatrix
} // namespace ZXing
//#include "datamatrix/DMDataBlock.h"
namespace ZXing::DataMatrix {
	class Version;

	/**
	 * <p>Encapsulates a block of data within a Data Matrix Code. Data Matrix Codes may split their data into
	 * multiple blocks, each of which is a unit of data and error-correction codewords. Each
	 * is represented by an instance of this class.</p>
	 */
	struct DataBlock {
		const int numDataCodewords = 0;
		ByteArray codewords;
	};

	/**
	 * <p>When Data Matrix Codes use multiple data blocks, they actually interleave the bytes of each of them.
	 * That is, the first byte of data block 1 to n is written, then the second bytes, and so on. This
	 * method will separate the data into original blocks.</p>
	 *
	 * @param rawCodewords bytes as read directly from the Data Matrix Code
	 * @param version version of the Data Matrix Code
	 * @param fix259 see https://github.com/zxing-cpp/zxing-cpp/issues/259
	 * @return DataBlocks containing original bytes, "de-interleaved" from representation in the
	 *         Data Matrix Code
	 */
	std::vector<DataBlock> GetDataBlocks(const ByteArray& rawCodewords, const Version& version, bool fix259 = false);
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMDecoder.h"
namespace ZXing {
	class DecoderResult;
	class BitMatrix;

	namespace DataMatrix {
		DecoderResult Decode(const BitMatrix& bits);
	} // DataMatrix
} // ZXing
//#include "datamatrix/DMDetector.h"
namespace ZXing {
	class BitMatrix;
	class DetectorResult;

	namespace DataMatrix {
		#ifdef __cpp_impl_coroutine
			using DetectorResults = Generator<DetectorResult>;
		#else
			using DetectorResults = DetectorResult;
		#endif
		DetectorResults Detect(const BitMatrix& image, bool tryHarder, bool tryRotate, bool isPure);
	} // DataMatrix
} // ZXing
//#include "datamatrix/DMECEncoder.h"
namespace ZXing {
	class ByteArray;
	namespace DataMatrix {
		class SymbolInfo;
		/**
		 * Creates and interleaves the ECC200 error correction for an encoded message.
		 *
		 * @param codewords  the codewords
		 * @param symbolInfo information about the symbol to be encoded
		 * @return the codewords with interleaved error correction.
		 */
		void EncodeECC200(ByteArray& codewords, const SymbolInfo& symbolInfo);
	} // DataMatrix
} // ZXing
//#include "datamatrix/DMSymbolShape.h"
namespace ZXing::DataMatrix {
	enum class SymbolShape {
		NONE,
		SQUARE,
		RECTANGLE,
	};
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMSymbolInfo.h"
namespace ZXing::DataMatrix {
	class SymbolInfo {
		bool _rectangular;
		int _dataCapacity;
		int _errorCodewords;
		int _matrixWidth;
		int _matrixHeight;
		int _dataRegions;
		int _rsBlockData;
		int _rsBlockError;
	public:
		constexpr SymbolInfo(bool rectangular, int dataCapacity, int errorCodewords, int matrixWidth, int matrixHeight, int dataRegions)
			: SymbolInfo(rectangular, dataCapacity, errorCodewords, matrixWidth, matrixHeight, dataRegions, dataCapacity, errorCodewords)
		{}
		constexpr SymbolInfo(bool rectangular, int dataCapacity, int errorCodewords, int matrixWidth, 
			int matrixHeight, int dataRegions, int rsBlockData, int rsBlockError) : _rectangular(rectangular),
			_dataCapacity(dataCapacity), _errorCodewords(errorCodewords), _matrixWidth(matrixWidth), _matrixHeight(matrixHeight),
			  _dataRegions(dataRegions), _rsBlockData(rsBlockData), _rsBlockError(rsBlockError)
		{}
		static const SymbolInfo* Lookup(int dataCodewords);
		static const SymbolInfo* Lookup(int dataCodewords, SymbolShape shape);
		static const SymbolInfo* Lookup(int dataCodewords, bool allowRectangular);
		static const SymbolInfo* Lookup(int dataCodewords, SymbolShape shape, int minWidth, int minHeight, int maxWidth, int maxHeight);
		int horizontalDataRegions() const;
		int verticalDataRegions() const;
		int symbolDataWidth() const { return horizontalDataRegions() * _matrixWidth; }
		int symbolDataHeight() const { return verticalDataRegions() * _matrixHeight; }
		int symbolWidth() const { return symbolDataWidth() + (horizontalDataRegions() * 2); }
		int symbolHeight() const { return symbolDataHeight() + (verticalDataRegions() * 2); }
		int matrixWidth() const { return _matrixWidth; }
		int matrixHeight() const { return _matrixHeight; }
		int codewordCount() const { return _dataCapacity + _errorCodewords; }
		int interleavedBlockCount() const { return _rsBlockData > 0 ? _dataCapacity / _rsBlockData : 10; /* Symbol 144 */ }
		int dataCapacity() const { return _dataCapacity; }
		int errorCodewords() const { return _errorCodewords; }
		int dataLengthForInterleavedBlock(int index) const
		{
			return _rsBlockData > 0 ? _rsBlockData : (index <= 8 ? 156 : 155); /* Symbol 144 */
		}
		int errorLengthForInterleavedBlock() const { return _rsBlockError; }
	};
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMHighLevelEncoder.h"
namespace ZXing {
	class ByteArray;
	namespace DataMatrix {
	enum class SymbolShape;
		/**
		* DataMatrix ECC 200 data encoder following the algorithm described in ISO/IEC 16022:200(E) in
		* annex S.
		*/
		ByteArray Encode(const std::wstring& msg);
		ByteArray Encode(const std::wstring& msg, CharacterSet encoding, SymbolShape shape, int minWidth, int minHeight, int maxWidth, int maxHeight);
	} // DataMatrix
} // ZXing
//#include "datamatrix/DMEncoderContext.h"
namespace ZXing::DataMatrix {
	class EncoderContext {
		std::string _msg;
		SymbolShape _shape = SymbolShape::NONE;
		int _minWidth = -1;
		int _minHeight = -1;
		int _maxWidth = -1;
		int _maxHeight = -1;
		ByteArray _codewords;
		int _pos = 0;
		int _newEncoding = -1;
		const SymbolInfo* _symbolInfo = nullptr;
		int _skipAtEnd = 0;
	public:
		explicit EncoderContext(std::string&& msg) : _msg(std::move(msg)) { _codewords.reserve(_msg.length()); }
		EncoderContext(const EncoderContext &) = delete;        // avoid copy by mistake
		void setSymbolShape(SymbolShape shape) 
		{
			_shape = shape;
		}
		void setSizeConstraints(int minWidth, int minHeight, int maxWidth, int maxHeight) 
		{
			_minWidth = minWidth;
			_minHeight = minHeight;
			_maxWidth = maxWidth;
			_maxHeight = maxHeight;
		}
		const std::string& message() const { return _msg; }
		void setSkipAtEnd(int count) { _skipAtEnd = count; }
		int currentPos() const { return _pos; }
		void setCurrentPos(int pos) { _pos = pos; }
		int currentChar() const { return _msg.at(_pos) & 0xff; }
		int nextChar() const { return _msg.at(_pos + 1) & 0xff; }
		const ByteArray& codewords() const { return _codewords; }
		int codewordCount() const { return Size(_codewords); }
		void addCodeword(uint8_t codeword) { _codewords.push_back(codeword); }
		void setNewEncoding(int encoding) { _newEncoding = encoding; }
		void clearNewEncoding() { _newEncoding = -1; }
		int newEncoding() const { return _newEncoding; }
		bool hasMoreCharacters() const { return _pos < totalMessageCharCount(); }
		int totalMessageCharCount() const { return narrow_cast<int>(_msg.length() - _skipAtEnd); }
		int remainingCharacters() const { return totalMessageCharCount() - _pos; }
		const SymbolInfo* updateSymbolInfo(int len) 
		{
			if(_symbolInfo == nullptr || len > _symbolInfo->dataCapacity()) {
				_symbolInfo = SymbolInfo::Lookup(len, _shape, _minWidth, _minHeight, _maxWidth, _maxHeight);
				if(_symbolInfo == nullptr) {
					throw std::invalid_argument("Can't find a symbol arrangement that matches the message. Data codewords: " + std::to_string(len));
				}
			}
			return _symbolInfo;
		}
		void resetSymbolInfo() { _symbolInfo = nullptr; }
		const SymbolInfo* symbolInfo() const { return _symbolInfo; }
	};
} // namespace ZXing::DataMatrix
//#include "datamatrix/DMWriter.h"
namespace ZXing {
	class BitMatrix;

	namespace DataMatrix {
		class Writer {
		public:
			Writer();
			Writer& setMargin(int margin) 
			{
				_quietZone = margin;
				return *this;
			}
			Writer& setShapeHint(SymbolShape shape) 
			{
				_shapeHint = shape;
				return *this;
			}
			Writer& setMinSize(int width, int height) 
			{
				_minWidth = width;
				_minHeight = height;
				return *this;
			}
			Writer& setMaxSize(int width, int height) 
			{
				_maxWidth = width;
				_maxHeight = height;
				return *this;
			}
			Writer& setEncoding(CharacterSet encoding) 
			{
				_encoding = encoding;
				return *this;
			}
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			SymbolShape _shapeHint;
			int _quietZone = 1, _minWidth = -1, _minHeight = -1, _maxWidth = -1, _maxHeight = -1;
			CharacterSet _encoding;
		};
	} // DataMatrix
} // ZXing
//
//#include "oned/ODReader.h"
namespace ZXing {
	class DecodeHints;

	namespace OneD {
		class RowReader;

		class Reader : public ZXing::Reader {
		public:
			explicit Reader(const DecodeHints& hints);
			~Reader() override;
			Result decode(const BinaryBitmap& image) const override;
			Results decode(const BinaryBitmap& image, int maxSymbols) const override;
		private:
			std::vector<std::unique_ptr<RowReader> > _readers;
		};
	} // OneD
} // ZXing
//#include "oned/ODRowReader.h"
/*
   Code39 : 1:2/3, 5+4+1 (0x3|2x1 wide) -> 12-15 mods, v1-? | ToNarrowWide(OMG 1) == *
   Codabar: 1:2/3, 4+3+1 (1x1|1x2|3x0 wide) -> 9-13 mods, v1-? | ToNarrowWide(OMG 2) == ABCD
   ITF    : 1:2/3, 5+5   (2x2 wide) -> mods, v6-?| .5, .38 == * | qz:10

   Code93 : 1-4, 3+3 -> 9 mods  v1-? | round to 1-4 == *
   Code128: 1-4, 3+3 -> 11 mods v1-? | .7, .25 == ABC | qz:10
   UPC/EAN: 1-4, 2+2 -> 7 mods  f    | .7, .48 == *
   UPC-A: 11d 95m = 3 + 6*4 + 5 + 6*4 + 3 = 59 | qz:3
   EAN-13: 12d 95m
   UPC-E: 6d, 3 + 6*4 + 6 = 33
   EAN-8: 8d, 3 + 4*4 + 5 + 4*4 + 3 = 43

   RSS14  : 1-8, finder: (15,2+3), symbol: (15/16,4+4) | .45, .2 (finder only), 14d
   code = 2xguard + 2xfinder + 4xsymbol = (96,23), stacked = 2x50 mods
   RSSExp.:  v?-74d/?-41c
 */
namespace ZXing {
	class DecodeHints;
	namespace OneD {
		/**
		 * Encapsulates functionality and implementation that is common to all families
		 * of one-dimensional barcodes.
		 */
		class RowReader {
		protected:
			const DecodeHints& _hints;
		public:
			explicit RowReader(const DecodeHints& hints) : _hints(hints) 
			{
			}
			explicit RowReader(DecodeHints&& hints) = delete;
			struct DecodingState 
			{
				virtual ~DecodingState() = default;
			};
			virtual ~RowReader() 
			{
			}
			virtual Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>& state) const = 0;
			/**
			 * Determines how closely a set of observed counts of runs of black/white values matches a given
			 * target pattern. This is reported as the ratio of the total variance from the expected pattern
			 * proportions across all pattern elements, to the length of the pattern.
			 *
			 * @param counters observed counters
			 * @param pattern expected pattern
			 * @param maxIndividualVariance The most any counter can differ before we give up
			 * @return ratio of total variance between counters and pattern compared to total pattern size
			 */
			template <typename CP, typename PP>
			static float PatternMatchVariance(const CP* counters, const PP* pattern, size_t length, float maxIndividualVariance)
			{
				int total = std::accumulate(counters, counters+length, 0);
				int patternLength = std::accumulate(pattern, pattern+length, 0);
				if(total < patternLength) {
					// If we don't even have one pixel per unit of bar width, assume this is too small
					// to reliably match, so fail:
					return std::numeric_limits<float>::max();
				}

				float unitBarWidth = (float)total / patternLength;
				maxIndividualVariance *= unitBarWidth;

				float totalVariance = 0.0f;
				for(size_t x = 0; x < length; ++x) {
					float variance = std::abs(counters[x] - pattern[x] * unitBarWidth);
					if(variance > maxIndividualVariance) {
						return std::numeric_limits<float>::max();
					}
					totalVariance += variance;
				}
				return totalVariance / total;
			}

			template <typename Counters, typename Pattern>
			static float PatternMatchVariance(const Counters& counters, const Pattern& pattern, float maxIndividualVariance) {
				assert(Size(counters) == Size(pattern));
				return PatternMatchVariance(std::data(counters), std::data(pattern), std::size(counters), maxIndividualVariance);
			}

			/**
			 * Attempts to decode a sequence of black/white lines into single
			 * digit.
			 *
			 * @param counters the counts of runs of observed black/white/black/... values
			 * @param patterns the list of patterns to compare the contents of counters to
			 * @param requireUnambiguousMatch the 'best match' must be better than all other matches
			 * @return The decoded digit index, -1 if no pattern matched
			 */
			template <typename Counters, typename Patterns>
			static int DecodeDigit(const Counters& counters, const Patterns& patterns, float maxAvgVariance,
				float maxIndividualVariance, bool requireUnambiguousMatch = true)
			{
				float bestVariance = maxAvgVariance; // worst variance we'll accept
				constexpr int INVALID_MATCH = -1;
				int bestMatch = INVALID_MATCH;
				for(int i = 0; i < Size(patterns); i++) {
					float variance = PatternMatchVariance(counters, patterns[i], maxIndividualVariance);
					if(variance < bestVariance) {
						bestVariance = variance;
						bestMatch = i;
					}
					else if(requireUnambiguousMatch && variance == bestVariance) {
						// if we find a second 'best match' with the same variance, we can not reliably report
						// to have a suitable match
						bestMatch = INVALID_MATCH;
					}
				}
				return bestMatch;
			}

			/**
			 * @brief NarrowWideThreshold calculates width thresholds to separate narrow and wide bars and spaces.
			 *
			 * This is useful for codes like Codabar, Code39 and ITF which distinguish between narrow and wide
			 * bars/spaces. Where wide ones are between 2 and 3 times as wide as the narrow ones.
			 *
			 * @param view containing one character
			 * @return threshold value for bars and spaces
			 */
			static BarAndSpaceI NarrowWideThreshold(const PatternView& view)
			{
				BarAndSpaceI m = {view[0], view[1]};
				BarAndSpaceI M = m;
				for(int i = 2; i < view.size(); ++i)
					UpdateMinMax(m[i], M[i], view[i]);

				BarAndSpaceI res;
				for(int i = 0; i < 2; ++i) {
					// check that
					//  a) wide <= 4 * narrow
					//  b) bars and spaces are not more than a factor of 2 (or 3 for the max) apart from each other
					if(M[i] > 4 * (m[i] + 1) || M[i] > 3 * M[i + 1] || m[i] > 2 * (m[i + 1] + 1))
						return {};
					// the threshold is the average of min and max but at least 1.5 * min
					res[i] = smax((m[i] + M[i]) / 2, m[i] * 3 / 2);
				}

				return res;
			}
			/**
			 * @brief ToNarrowWidePattern takes a PatternView, calculates a NarrowWideThreshold and returns int where a '0'
			 *bit
			 * means narrow and a '1' bit means 'wide'.
			 */
			static int NarrowWideBitPattern(const PatternView& view)
			{
				const auto threshold = NarrowWideThreshold(view);
				if(!threshold.isValid())
					return -1;
				int pattern = 0;
				for(int i = 0; i < view.size(); ++i) {
					if(view[i] > threshold[i] * 2)
						return -1;
					AppendBit(pattern, view[i] > threshold[i]);
				}
				return pattern;
			}
			/**
			 * @brief each bar/space is 1-4 modules wide, we have N bars/spaces, they are SUM modules wide in total
			 */
			template <int LEN, int SUM> static int OneToFourBitPattern(const PatternView& view)
			{
				// TODO: make sure none of the elements in the normalized pattern exceeds 4
				return ToInt(NormalizedPattern<LEN, SUM>(view));
			}
			/**
			 * @brief Lookup the pattern in the table and return the character in alphabet at the same index.
			 * @returns 0 if pattern is not found. Used to be -1 but that fails on systems where char is unsigned.
			 */
			template <typename INDEX, typename ALPHABET> static char LookupBitPattern(int pattern, const INDEX& table, const ALPHABET& alphabet)
			{
				int i = IndexOf(table, pattern);
				return i == -1 ? 0 : alphabet[i];
			}
			template <typename INDEX, typename ALPHABET> static char DecodeNarrowWidePattern(const PatternView& view, const INDEX& table, const ALPHABET& alphabet)
			{
				return LookupBitPattern(NarrowWideBitPattern(view), table, alphabet);
			}
		};
		template <typename Range> Result DecodeSingleRow(const RowReader& reader, const Range& range)
		{
			PatternRow row;
			GetPatternRow(range, row);
			PatternView view(row);
			std::unique_ptr<RowReader::DecodingState> state;
			return reader.decodePattern(0, view, state);
		}
	} // OneD
} // ZXing
//#include "oned/ODCodabarReader.h"
namespace ZXing::OneD {
	class CodabarReader : public RowReader {
	public:
		using RowReader::RowReader;

		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>& state) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODCode128Reader.h"
namespace ZXing::OneD {
	class Code128Reader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODCode39Reader.h"
namespace ZXing::OneD {
	class Code39Reader : public RowReader {
	public:
		/**
		* Creates a reader that can be configured to check the last character as a check digit,
		* or optionally attempt to decode "extended Code 39" sequences that are used to encode
		* the full ASCII character set.
		*/
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODCode93Reader.h"
namespace ZXing::OneD {
	class Code93Reader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODDataBarExpandedReader.h"
namespace ZXing::OneD {
	/**
	* Decodes DataBarExpandedReader (formerly known as RSS) sybmols, including truncated and stacked variants. See ISO/IEC 24724:2006.
	*/
	class DataBarExpandedReader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>& state) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODDataBarReader.h"
namespace ZXing::OneD {
	/**
	* Decodes DataBar (formerly known as RSS) sybmols, including truncated and stacked variants. See ISO/IEC 24724:2006.
	*/
	class DataBarReader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>& state) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODITFReader.h"
namespace ZXing::OneD {
	/**
	* <p>Implements decoding of the ITF format, or Interleaved Two of Five.</p>
	*
	* <p>This Reader will scan ITF barcodes of certain lengths only.
	* At the moment it reads length >= 6. Not all lengths are scanned, especially shorter ones, to avoid false positives.
	* This in turn is due to a lack of required checksum function.</p>
	*
	* <p>The checksum is optional and is only checked if the validateITFCheckSum hint is given.</p>
	*
	* <p><a href="http://en.wikipedia.org/wiki/Interleaved_2_of_5">http://en.wikipedia.org/wiki/Interleaved_2_of_5</a>
	* is a great reference for Interleaved 2 of 5 information.</p>
	*/
	class ITFReader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODMultiUPCEANReader.h"
namespace ZXing::OneD {
	/**
	* @brief A reader that can read all available UPC/EAN formats.
	*/
	class MultiUPCEANReader : public RowReader {
	public:
		using RowReader::RowReader;
		Result decodePattern(int rowNumber, PatternView& next, std::unique_ptr<DecodingState>&) const override;
	};
} // namespace ZXing::OneD
//#include "oned/ODUPCEANCommon.h"
namespace ZXing::OneD::UPCEANCommon {
	using Digit = std::array<int, 4>;
	/**
	 * Start/end guard pattern.
	 */
	extern const std::array<int, 3> START_END_PATTERN;
	/**
	 * "Odd", or "L" patterns used to encode UPC/EAN digits.
	 */
	extern const std::array<Digit, 10> L_PATTERNS;
	/**
	 * Pattern marking the middle of a UPC/EAN pattern, separating the two halves.
	 */
	extern const std::array<int, 5> MIDDLE_PATTERN;
	/**
	 * As above but also including the "even", or "G" patterns used to encode UPC/EAN digits.
	 */
	extern const std::array<Digit, 20> L_AND_G_PATTERNS;
	/**
	 * UPCE end guard pattern (== MIDDLE_PATTERN + single module black bar)
	 */
	extern const std::array<int, 6> UPCE_END_PATTERN;
	/**
	 * See {@link #L_AND_G_PATTERNS}; these values similarly represent patterns of
	 * even-odd parity encodings of digits that imply both the number system (0 or 1)
	 * used (index / 10), and the check digit (index % 10).
	 */
	extern const std::array<int, 20> NUMSYS_AND_CHECK_DIGIT_PATTERNS;

	template <size_t N, typename T> std::array<int, N> DigitString2IntArray(const std::basic_string<T>& in, int checkDigit = -1)
	{
		static_assert(N == 8 || N == 13, "invalid UPC/EAN length");
		if(in.size() != N && in.size() != N - 1)
			throw std::invalid_argument("Invalid input string length");
		std::array<int, N> out = {};
		for(size_t i = 0; i < in.size(); ++i) {
			out[i] = in[i] - '0';
			if(out[i] < 0 || out[i] > 9)
				throw std::invalid_argument("Contents must contain only digits: 0-9");
		}
		if(checkDigit == -1)
			checkDigit = GTIN::ComputeCheckDigit(in, N == in.size());
		if(in.size() == N - 1)
			out.back() = checkDigit - '0';
		else if(in.back() != checkDigit)
			throw std::invalid_argument("Checksum error");
		return out;
	}
	/**
	 * Expands a UPC-E value back into its full, equivalent UPC-A code value.
	 *
	 * @param upce UPC-E code as string of digits
	 * @return equivalent UPC-A code as string of digits
	 */
	template <typename StringT> StringT ConvertUPCEtoUPCA(const StringT& upce)
	{
		if(upce.length() < 7)
			return upce;
		auto upceChars = upce.substr(1, 6);
		StringT result;
		result.reserve(12);
		result += upce[0];
		auto lastChar = upceChars[5];
		switch(lastChar) {
			case '0':
			case '1':
			case '2':
				result += upceChars.substr(0, 2);
				result += lastChar;
				result += StringT(4, '0');
				result += upceChars.substr(2, 3);
				break;
			case '3':
				result += upceChars.substr(0, 3);
				result += StringT(5, '0');
				result += upceChars.substr(3, 2);
				break;
			case '4':
				result += upceChars.substr(0, 4);
				result += StringT(5, '0');
				;
				result += upceChars[4];
				break;
			default:
				result += upceChars.substr(0, 5);
				result += StringT(4, '0');
				result += lastChar;
				break;
		}
		// Only append check digit in conversion if supplied
		if(upce.length() >= 8) {
			result += upce[7];
		}
		return result;
	}
} // namespace ZXing::OneD::UPCEANCommon
//#include "oned/ODCodabarWriter.h"
namespace ZXing {
	class BitMatrix;

	namespace OneD {
		/**
		 * This class renders CodaBar as {@code boolean[]}.
		 *
		 * @author dsbnatut@gmail.com (Kazuki Nishiura)
		 */
		class CodabarWriter {
		public:
			CodabarWriter& setMargin(int sidesMargin) 
			{
				_sidesMargin = sidesMargin; return *this;
			}
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODWriterHelper.h"
namespace ZXing::OneD {
	/**
	* <p>Encapsulates functionality and implementation that is common to one-dimensional barcodes.</p>
	*
	* @author dsbnatut@gmail.com (Kazuki Nishiura)
	*/
	class WriterHelper {
		static int AppendPattern(std::vector<bool>& target, int pos, const int* pattern, size_t patternCount, bool startColor);
	public:
		/**
		* @return a byte array of horizontal pixels (0 = white, 1 = black)
		*/
		static BitMatrix RenderResult(const std::vector<bool>& code, int width, int height, int sidesMargin);
		/**
		* @param target encode black/white pattern into this array
		* @param pos position to start encoding at in {@code target}
		* @param pattern lengths of black/white runs to encode
		* @param startColor starting color - false for white, true for black
		* @return the number of elements added to target.
		*/
		template <typename Container> static int AppendPattern(std::vector<bool>& target, int pos, const Container& pattern, bool startColor) 
		{
			return AppendPattern(target, pos, pattern.data(), pattern.size(), startColor);
		}
	};
} // namespace ZXing::OneD
//#include "oned/ODCode39Writer.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders a CODE39 code as a {@link BitMatrix}.
		* 
		* @author erik.barbara@gmail.com (Erik Barbara)
		*/
		class Code39Writer {
		public:
			Code39Writer& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODCode93Writer.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders a CODE93 code as a BitMatrix
		*/
		class Code93Writer {
		public:
			Code93Writer& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODCode128Patterns.h"
namespace ZXing::OneD::Code128 {
	extern const std::array<std::array<int, 6>, 107> CODE_PATTERNS;
} // namespace ZXing::OneD::Code128
//#include "oned/ODCode128Writer.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders a CODE128 code as a {@link BitMatrix}.
		*
		* @author erik.barbara@gmail.com (Erik Barbara)
		*/
		class Code128Writer {
		public:
			Code128Writer& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODDataBarCommon.h"
namespace ZXing::OneD::DataBar {
	inline bool IsFinder(int a, int b, int c, int d, int e)
	{
		//  a,b,c,d,e, g | sum(a..e) = 15
		//  ------------
		//  1,1,2
		//	| | |,1,1, 1
		//	3,8,9

		// use only pairs of bar+space to limit effect of poor threshold:
		// b+c can be 10, 11 or 12 modules, d+e is always 2
		int w = 2 * (b + c), n = d + e;
		// the offsets (5 and 2) are there to reduce quantization effects for small module sizes
		// TODO: review after switch to sub-pixel bar width calculation
		bool x = (w + 5 > 9 * n) &&
			(w - 5 < 13 * n) &&
	//			 (b < 5 + 9 * d) &&
	//			 (c < 5 + 10 * e) &&
			(a < 2 + 4 * e) &&
			(4 * a > n);
	#if !defined(NDEBUG) && 0
		printf("[");
		for(bool v :
			{w + 5 > 9 * n,
			 w - 5 < 13 * n,
	//		  b < 5 + 9 * d,
	//		  c < 5 + 10 * e,
			 a < 2 + 4 * e,
			 4 * a > n})
			printf(" %d", v);
		printf("]"); fflush(stdout);
	#endif
		return x;
	};

	inline PatternView Finder(const PatternView& view) { return view.subView(8, 5); }
	inline PatternView LeftChar(const PatternView& view) { return view.subView(0, 8); }
	inline PatternView RightChar(const PatternView& view) { return view.subView(13, 8); }
	inline float ModSizeFinder(const PatternView& view) { return Finder(view).sum() / 15.f; }
	inline bool IsGuard(int a, int b)
	{
		//	printf(" (%d, %d)", a, b);
		return a > b * 3 / 4 - 2 && a < b * 5 / 4 + 2;
	}
	inline bool IsCharacter(const PatternView& view, int modules, float modSizeRef)
	{
		float err = std::abs(float(view.sum()) / modules / modSizeRef - 1);
		return err < 0.1f;
	}

	struct Character {
		int value = -1;
		int checksum = 0;
		operator bool() const noexcept { return value != -1; }
		bool operator==(const Character& o) const noexcept { return value == o.value && checksum == o.checksum; }
		bool operator!=(const Character& o) const { return !(*this == o); }
	};
	struct Pair {
		Character left;
		Character right;
		int finder = 0;
		int xStart = -1;
		int xStop = 1;
		int y = -1;
		int count = 1;

		operator bool() const noexcept { return finder != 0; }
		bool operator==(const Pair& o) const noexcept { return finder == o.finder && left == o.left && right == o.right; }
		bool operator!=(const Pair& o) const noexcept { return !(*this == o); }
	};
	struct PairHash {
		std::size_t operator()(const Pair& p) const noexcept
		{
			return p.left.value ^ p.left.checksum ^ p.right.value ^ p.right.checksum ^ p.finder;
		}
	};

	constexpr int FULL_PAIR_SIZE = 8 + 5 + 8;
	constexpr int HALF_PAIR_SIZE = 8 + 5 + 2; // half has to be followed by a guard pattern

	template <typename T> int ParseFinderPattern(const PatternView& view, bool reversed, T l2rPattern, T r2lPattern)
	{
		constexpr float MAX_AVG_VARIANCE        = 0.2f;
		constexpr float MAX_INDIVIDUAL_VARIANCE = 0.45f;
		int i = 1 + RowReader::DecodeDigit(view, reversed ? r2lPattern : l2rPattern, MAX_AVG_VARIANCE, MAX_INDIVIDUAL_VARIANCE, true);
		return reversed ? -i : i;
	}

	using Array4I = std::array<int, 4>;

	bool ReadDataCharacterRaw(const PatternView& view, int numModules, bool reversed, Array4I& oddPattern, Array4I& evnPattern);
	int GetValue(const Array4I& widths, int maxWidth, bool noNarrow);
	Position EstimatePosition(const Pair& first, const Pair& last);
	int EstimateLineCount(const Pair& first, const Pair& last);
} // namespace ZXing::OneD::DataBar
//#include "oned/ODDataBarExpandedBitDecoder.h"
namespace ZXing {
	class BitArray;

	namespace OneD::DataBar {
		std::string DecodeExpandedBits(const BitArray& bits);
	} // namespace OneD::DataBar
} // namespace ZXing
//#include "oned/ODEAN8Writer.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders an EAN8 code as a {@link BitMatrix}.
		*
		* @author aripollak@gmail.com (Ari Pollak)
		*/
		class EAN8Writer {
		public:
			EAN8Writer& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODEAN13Writer.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders an EAN8 code as a {@link BitMatrix}.
		*
		* @author aripollak@gmail.com (Ari Pollak)
		*/
		class EAN13Writer {
		public:
			EAN13Writer& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODITFWriter.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders a ITF code as a {@link BitMatrix}.
		*
		* @author erik.barbara@gmail.com (Erik Barbara)
		*/
		class ITFWriter {
		public:
			ITFWriter& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODUPCAWriter.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders a UPC-A code as a {@link BitMatrix}.
		*
		* @author qwandor@google.com (Andrew Walbran)
		*/
		class UPCAWriter {
		public:
			UPCAWriter& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "oned/ODUPCEWriter.h"
namespace ZXing {
	class BitMatrix;
	namespace OneD {
		/**
		* This object renders an UPC-E code as a {@link BitMatrix}.
		*
		* @author 0979097955s@gmail.com (RX)
		*/
		class UPCEWriter {
		public:
			UPCEWriter& setMargin(int sidesMargin) { _sidesMargin = sidesMargin; return *this; }
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _sidesMargin = -1;
		};
	} // OneD
} // ZXing
//#include "pdf417/PDFReader.h"
namespace ZXing::Pdf417 {
	/**
	* This implementation can detect and decode PDF417 codes in an image.
	*
	* @author Guenther Grau
	*/
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;
		Result decode(const BinaryBitmap& image) const override;
		Results decode(const BinaryBitmap& image, int maxSymbols) const override;
	};
} // namespace ZXing::Pdf417
//#include "pdf417/PDFBarcodeValue.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		 * @author Guenther Grau
		 */
		class BarcodeValue {
			std::map<int, int> _values;
		public:
			/**
			 * Add an occurrence of a value
			 */
			void setValue(int value);
			/**
			 * Determines the maximum occurrence of a set value and returns all values which were set with this occurrence.
			 * @return an array of int, containing the values with the highest occurrence, or null, if no value was set
			 */
			std::vector<int> value() const;
			int confidence(int value) const;
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFBoundingBox.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		 * @author Guenther Grau
		 */
		class BoundingBox {
			int _imgWidth;
			int _imgHeight;
			Nullable<ResultPoint> _topLeft;
			Nullable<ResultPoint> _bottomLeft;
			Nullable<ResultPoint> _topRight;
			Nullable<ResultPoint> _bottomRight;
			int _minX;
			int _maxX;
			int _minY;
			int _maxY;

		public:
			BoundingBox();
			int minX() const { return _minX; }
			int maxX() const { return _maxX; }
			int minY() const { return _minY; }
			int maxY() const { return _maxY; }
			Nullable<ResultPoint> topLeft() const { return _topLeft; }
			Nullable<ResultPoint> topRight() const { return _topRight; }
			Nullable<ResultPoint> bottomLeft() const { return _bottomLeft; }
			Nullable<ResultPoint> bottomRight() const { return _bottomRight; }
			static bool Create(int imgWidth, int imgHeight, const Nullable<ResultPoint>& topLeft,
				const Nullable<ResultPoint>& bottomLeft, const Nullable<ResultPoint>& topRight, const Nullable<ResultPoint>& bottomRight, BoundingBox& result);
			static bool Merge(const Nullable<BoundingBox>& leftBox, const Nullable<BoundingBox>& rightBox, Nullable<BoundingBox>& result);
			static bool AddMissingRows(const BoundingBox&box, int missingStartRows, int missingEndRows, bool isLeft, BoundingBox& result);
		private:
			void calculateMinMaxValues();
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFCodewordDecoder.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		* @author Guenther Grau
		* @author creatale GmbH (christoph.schulz@creatale.de)
		*/
		class CodewordDecoder {
		public:
			static constexpr const int NUMBER_OF_CODEWORDS = 929;
			// Maximum Codewords (Data + Error).
			static constexpr const int MAX_CODEWORDS_IN_BARCODE = NUMBER_OF_CODEWORDS - 1;
			// One left row indication column + max 30 data columns + one right row indicator column
			//public static final int MAX_CODEWORDS_IN_ROW = 32;
			static constexpr const int MODULES_IN_CODEWORD = 17;
			static constexpr const int BARS_IN_MODULE = 8;
			/**
			* @param symbol encoded symbol to translate to a codeword
			* @return the codeword corresponding to the symbol.
			*/
			static int GetCodeword(int symbol);
			static int GetDecodedValue(const std::array<int, BARS_IN_MODULE>& moduleBitCount);
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFDecoder.h"
namespace ZXing {
	class DecoderResult;
	namespace Pdf417 {
		DecoderResult Decode(const std::vector<int>& codewords);
	} // namespace Pdf417
} // namespace ZXing
//#include "pdf417/PDFDecoderResultExtra.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		* @author Guenther Grau
		*/
		class DecoderResultExtra : public CustomData {
			int _segmentIndex = -1;
			std::string _fileId;
			std::vector<int> _optionalData;
			bool _lastSegment = false;
			int _segmentCount = -1;
			std::string _sender;
			std::string _addressee;
			std::string _fileName;
			int64_t _fileSize = -1;
			int64_t _timestamp = -1;
			int _checksum = -1;
		public:
			int segmentIndex() const { return _segmentIndex; }
			// -1 if not set
			void setSegmentIndex(int segmentIndex) 
			{
				_segmentIndex = segmentIndex;
			}
			std::string fileId() const { return _fileId; }
			void setFileId(const std::string& fileId) 
			{
				_fileId = fileId;
			}
			const std::vector<int>& optionalData() const { return _optionalData; }
			void setOptionalData(const std::vector<int>& optionalData) 
			{
				_optionalData = optionalData;
			}
			bool isLastSegment() const { return _lastSegment; }
			void setLastSegment(bool lastSegment) { _lastSegment = lastSegment; }
			int segmentCount() const { return _segmentCount; }
			void setSegmentCount(int segmentCount) { _segmentCount = segmentCount; }
			std::string sender() const { return _sender; } // UTF-8
			void setSender(const std::string& sender) { _sender = sender; }
			std::string addressee() const { return _addressee; } // UTF-8
			void setAddressee(const std::string& addressee) { _addressee = addressee; }
			std::string fileName() const { return _fileName; } // UTF-8
			void setFileName(const std::string& fileName) { _fileName = fileName; }
			// -1 if not set
			int64_t fileSize() const { return _fileSize; }
			void setFileSize(int64_t fileSize) { _fileSize = fileSize; }
			 // 16-bit CRC checksum using CCITT-16, -1 if not set
			int checksum() const { return _checksum; }
			void setChecksum(int checksum) { _checksum = checksum; }
			// Unix epock timestamp, elapsed seconds since 1970-01-01, -1 if not set
			int64_t timestamp() const { return _timestamp; }
			void setTimestamp(int64_t timestamp) { _timestamp = timestamp; }
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFBarcodeMetadata.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		 * @author Guenther Grau
		 */
		class BarcodeMetadata {
			int _columnCount = 0;
			int _errorCorrectionLevel = 0;
			int _rowCountUpperPart = 0;
			int _rowCountLowerPart = 0;
		public:
			BarcodeMetadata() = default;
			BarcodeMetadata(int columnCount, int rowCountUpperPart, int rowCountLowerPart, int errorCorrectionLevel)
				: _columnCount(columnCount), _errorCorrectionLevel(errorCorrectionLevel), _rowCountUpperPart(rowCountUpperPart),
				_rowCountLowerPart(rowCountLowerPart)
			{
			}
			int columnCount() const { return _columnCount; }
			int errorCorrectionLevel() const { return _errorCorrectionLevel; }
			int rowCount() const { return _rowCountUpperPart + _rowCountLowerPart; }
			int rowCountUpperPart() const { return _rowCountUpperPart; }
			int rowCountLowerPart() const { return _rowCountLowerPart; }
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFCodeword.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		* @author Guenther Grau
		*/
		class Codeword {
			static const int BARCODE_ROW_UNKNOWN = -1;
			int _startX = 0;
			int _endX = 0;
			int _bucket = 0;
			int _value = 0;
			int _rowNumber = BARCODE_ROW_UNKNOWN;
		public:
			Codeword() {}
			Codeword(int startX, int endX, int bucket, int value) : _startX(startX), _endX(endX), _bucket(bucket), _value(value) {}
			bool hasValidRowNumber() const { return isValidRowNumber(_rowNumber); }
			bool isValidRowNumber(int rowNumber) const { return rowNumber != BARCODE_ROW_UNKNOWN && _bucket == (rowNumber % 3) * 3; }
			void setRowNumberAsRowIndicatorColumn() 
			{
				_rowNumber = (_value / 30) * 3 + _bucket / 3;
			}
			int width() const { return _endX - _startX; }
			int startX() const { return _startX; }
			int endX() const { return _endX; }
			int bucket() const { return _bucket; }
			int value() const { return _value; }
			int rowNumber() const { return _rowNumber; }
			void setRowNumber(int rowNumber) 
			{
				_rowNumber = rowNumber;
			}
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFDetectionResultColumn.h"
namespace ZXing {
	namespace Pdf417 {
		class BarcodeMetadata;

		/**
		* @author Guenther Grau
		*/
		class DetectionResultColumn {
		public:
			enum class RowIndicator {
				None,
				Left,
				Right,
			};
			DetectionResultColumn() {}
			explicit DetectionResultColumn(const BoundingBox& boundingBox, RowIndicator rowInd = RowIndicator::None);
			bool isRowIndicator() const { return _rowIndicator != RowIndicator::None; }
			bool isLeftRowIndicator() const { return _rowIndicator == RowIndicator::Left; }
			Nullable<Codeword> codewordNearby(int imageRow) const;
			int imageRowToCodewordIndex(int imageRow) const { return imageRow - _boundingBox.minY(); }
			void setCodeword(int imageRow, Codeword codeword) { _codewords[imageRowToCodewordIndex(imageRow)] = codeword; }
			Nullable<Codeword> codeword(int imageRow) const { return _codewords[imageRowToCodewordIndex(imageRow)]; }
			const BoundingBox& boundingBox() const { return _boundingBox; }
			const std::vector<Nullable<Codeword>>& allCodewords() const { return _codewords; }
			std::vector<Nullable<Codeword>>& allCodewords() { return _codewords; }
			void adjustCompleteIndicatorColumnRowNumbers(const BarcodeMetadata& barcodeMetadata);
			bool getRowHeights(std::vector<int>& result); // not const, since it modifies object's state
			bool getBarcodeMetadata(BarcodeMetadata& result); // not const, since it modifies object's state
		private:
			BoundingBox _boundingBox;
			std::vector<Nullable<Codeword>> _codewords;
			RowIndicator _rowIndicator = RowIndicator::None;

			void setRowNumbers();
			void adjustIncompleteIndicatorColumnRowNumbers(const BarcodeMetadata& barcodeMetadata);
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFDetectionResult.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		* @author Guenther Grau
		*/
		class DetectionResult {
			BarcodeMetadata _barcodeMetadata;
			std::vector<Nullable<DetectionResultColumn>> _detectionResultColumns;
			Nullable<BoundingBox> _boundingBox;
		public:
			DetectionResult() = default;
			DetectionResult(const BarcodeMetadata& barcodeMetadata, const Nullable<BoundingBox>& boundingBox);
			void init(const BarcodeMetadata& barcodeMetadata, const Nullable<BoundingBox>& boundingBox);
			const std::vector<Nullable<DetectionResultColumn>> & allColumns();
			int barcodeColumnCount() const { return _barcodeMetadata.columnCount(); }
			int barcodeRowCount() const { return _barcodeMetadata.rowCount(); }
			int barcodeECLevel() const { return _barcodeMetadata.errorCorrectionLevel(); }
			void setBoundingBox(const BoundingBox& boundingBox) 
			{
				_boundingBox = boundingBox;
			}
			const Nullable<BoundingBox> & getBoundingBox() const { return _boundingBox; }
			void setBoundingBox(const Nullable<BoundingBox>& box) 
			{
				_boundingBox = box;
			}
			void setColumn(int barcodeColumn, const Nullable<DetectionResultColumn>& detectionResultColumn) 
			{
				_detectionResultColumns[barcodeColumn] = detectionResultColumn;
			}
			const Nullable<DetectionResultColumn>& column(int barcodeColumn) const { return _detectionResultColumns[barcodeColumn]; }
			Nullable<DetectionResultColumn>& column(int barcodeColumn) { return _detectionResultColumns[barcodeColumn]; }
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFDetector.h"
namespace ZXing {
	class BitMatrix;
	class BinaryBitmap;

	namespace Pdf417 {
		/**
		* <p>Encapsulates logic that can detect a PDF417 Code in an image, even if the
		* PDF417 Code is rotated or skewed, or partially obscured.< / p>
		*
		* @author SITA Lab(kevin.osullivan@sita.aero)
		* @author dswitkin@google.com(Daniel Switkin)
		* @author Guenther Grau
		*/
		class Detector {
		public:
			struct Result {
				std::shared_ptr<const BitMatrix> bits;
				std::list<std::array<Nullable<ResultPoint>, 8>> points;
				int rotation = -1;
			};
			static Result Detect(const BinaryBitmap& image, bool multiple, bool tryRotate);
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFCompaction.h"
namespace ZXing {
	namespace Pdf417 {
		enum class Compaction {
			AUTO,
			TEXT,
			BYTE,
			NUMERIC
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFEncoder.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		 * @author Jacob Haynes
		 */
		class BarcodeRow {
			std::vector<bool> _row;
			int _currentLocation = 0; // A tacker for position in the bar
		public:
			explicit BarcodeRow(int width = 0) : _row(width, false) 
			{
			}
			void init(int width) 
			{
				_row.resize(width, false);
				_currentLocation = 0;
			}
			void set(int x, bool black) 
			{
				_row.at(x) = black;
			}
			/**
			 * @param black A boolean which is true if the bar black false if it is white
			 * @param width How many spots wide the bar is.
			 */
			void addBar(bool black, int width) 
			{
				for(int ii = 0; ii < width; ii++) {
					_row.at(_currentLocation++) = black;
				}
			}
			/**
			 * This function scales the row
			 *
			 * @param scale How much you want the image to be scaled, must be greater than or equal to 1.
			 * @return the scaled row
			 */
			void getScaledRow(int scale, std::vector<bool>& output) const 
			{
				output.resize(_row.size() * scale);
				for(size_t i = 0; i < output.size(); ++i) {
					output[i] = _row[i / scale];
				}
			}
		};
		/**
		 * Holds all of the information for a barcode in a format where it can be easily accessible
		 *
		 * @author Jacob Haynes
		 */
		class BarcodeMatrix {
			std::vector<BarcodeRow> _matrix;
			int _width = 0;
			int _currentRow = -1;
		public:
			BarcodeMatrix() 
			{
			}
			/**
			 * @param height the height of the matrix (Rows)
			 * @param width  the width of the matrix (Cols)
			 */
			BarcodeMatrix(int height, int width) 
			{
				init(height, width);
			}
			void init(int height, int width) 
			{
				_matrix.resize(height);
				for(int i = 0; i < height; ++i) {
					_matrix[i].init((width + 4) * 17 + 1);
				}
				_width = width * 17;
				_currentRow = -1;
			}
			void set(int x, int y, bool value) 
			{
				_matrix[y].set(x, value);
			}
			void startRow() { ++_currentRow; }
			const BarcodeRow& currentRow() const { return _matrix[_currentRow]; }
			BarcodeRow& currentRow() { return _matrix[_currentRow]; }
			void getScaledMatrix(int xScale, int yScale, std::vector<std::vector<bool> >& output)
			{
				output.resize(_matrix.size() * yScale);
				int yMax = Size(output);
				for(int i = 0; i < yMax; i++) {
					_matrix[i / yScale].getScaledRow(xScale, output[yMax - i - 1]);
				}
			}
		};
		/**
		 * Top-level class for the logic part of the PDF417 implementation.
		 * C++ port: this class was named PDF417 in Java code. Since that name
		 * does say much in the context of PDF417 writer, it's renamed here Encoder
		 * to follow the same naming convention with other modules.
		 */
		class Encoder {
		public:
			explicit Encoder(bool compact = false) : _compact(compact)  
			{
			}
			BarcodeMatrix generateBarcodeLogic(const std::wstring& msg, int errorCorrectionLevel) const;
			/**
			 * Sets max/min row/col values
			 *
			 * @param maxCols maximum allowed columns
			 * @param minCols minimum allowed columns
			 * @param maxRows maximum allowed rows
			 * @param minRows minimum allowed rows
			 */
			void setDimensions(int minCols, int maxCols, int minRows, int maxRows) 
			{
				_minCols = minCols;
				_maxCols = maxCols;
				_minRows = minRows;
				_maxRows = maxRows;
			}
			/**
			 * @param compaction compaction mode to use
			 */
			void setCompaction(Compaction compaction) { _compaction = compaction; }
			/**
			 * @param compact if true, enables compaction
			 */
			void setCompact(bool compact) { _compact = compact; }
			/**
			 * @param encoding sets character encoding to use
			 */
			void setEncoding(CharacterSet encoding) { _encoding = encoding; }
			static int GetRecommendedMinimumErrorCorrectionLevel(int n);
		private:
			bool _compact;
			Compaction _compaction = Compaction::AUTO;
			CharacterSet _encoding = CharacterSet::ISO8859_1;
			int _minCols = 2;
			int _maxCols = 30;
			int _minRows = 2;
			int _maxRows = 30;
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFHighLevelEncoder.h"
namespace ZXing {
	namespace Pdf417 {
		enum class Compaction;
		/**
		* PDF417 high-level encoder following the algorithm described in ISO/IEC 15438:2001(E) in
		* annex P.
		*/
		class HighLevelEncoder {
		public:
			static std::vector<int> EncodeHighLevel(const std::wstring& msg, Compaction compaction, CharacterSet encoding);
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFModulusPoly.h"
namespace ZXing {
	namespace Pdf417 {
		class ModulusGF;

		/**
		* @author Sean Owen
		* @see com.google.zxing.common.reedsolomon.GenericGFPoly
		*/
		class ModulusPoly {
			const ModulusGF* _field = nullptr;
			std::vector<int> _coefficients;
		public:
			// Build a invalid object, so that this can be used in container or return by reference,
			// any access to invalid object is undefined behavior.
			ModulusPoly() = default;
			ModulusPoly(const ModulusGF& field, const std::vector<int>& coefficients);
			const std::vector<int>& coefficients() const { return _coefficients; }
			/**
			* @return degree of this polynomial
			*/
			int degree() const { return Size(_coefficients) - 1; }
			/**
			* @return true iff this polynomial is the monomial "0"
			*/
			bool isZero() const { return _coefficients.at(0) == 0; }
			/**
			* @return coefficient of x^degree term in this polynomial
			*/
			int coefficient(int degree) const { return _coefficients.at(_coefficients.size() - 1 - degree); }
			/**
			* @return evaluation of this polynomial at a given point
			*/
			int evaluateAt(int a) const;
			ModulusPoly add(const ModulusPoly& other) const;
			ModulusPoly subtract(const ModulusPoly& other) const;
			ModulusPoly multiply(const ModulusPoly& other) const;
			ModulusPoly negative() const;
			ModulusPoly multiply(int scalar) const;
			ModulusPoly multiplyByMonomial(int degree, int coefficient) const;
			void divide(const ModulusPoly& other, ModulusPoly& quotient, ModulusPoly& remainder) const;
			friend void swap(ModulusPoly& a, ModulusPoly& b)
			{
				std::swap(a._field, b._field);
				std::swap(a._coefficients, b._coefficients);
			}
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFModulusGF.h"
namespace ZXing {
	namespace Pdf417 {
		/**
		 * <p>A field based on powers of a generator integer, modulo some modulus.< / p>
		 *
		 * @author Sean Owen
		 * @see com.google.zxing.common.reedsolomon.GenericGF
		 */
		class ModulusGF {
			int _modulus;
			std::vector<short> _expTable;
			std::vector<short> _logTable;
			ModulusPoly _zero;
			ModulusPoly _one;
			// avoid using the '%' modulo operator => ReedSolomon computation is more than twice as fast
			// see also https://stackoverflow.com/a/33333636/2088798
			static int fast_mod(int a, int d) { return a < d ? a : a - d; }
		public:
			ModulusGF(int modulus, int generator);
			const ModulusPoly& zero() const { return _zero; }
			const ModulusPoly& one() const { return _one; }
			ModulusPoly buildMonomial(int degree, int coefficient) const;
			int add(int a, int b) const { return fast_mod(a + b, _modulus); }
			int subtract(int a, int b) const { return fast_mod(_modulus + a - b, _modulus); }
			int exp(int a) const { return _expTable.at(a); }
			int log(int a) const
			{
				if(a == 0) {
					throw std::invalid_argument("a == 0");
				}
				return _logTable[a];
			}
			int inverse(int a) const 
			{
				if(a == 0) {
					throw std::invalid_argument("a == 0");
				}
				return _expTable[_modulus - _logTable[a] - 1];
			}
			int multiply(int a, int b) const 
			{
				if(a == 0 || b == 0) {
					return 0;
				}
		#ifdef ZX_REED_SOLOMON_USE_MORE_MEMORY_FOR_SPEED
				return _expTable[_logTable[a] + _logTable[b]];
		#else
				return _expTable[fast_mod(_logTable[a] + _logTable[b], _modulus - 1)];
		#endif
			}
			int size() const { return _modulus; }
		};
	} // Pdf417
} // ZXing
//#include "pdf417/PDFScanningDecoder.h"
namespace ZXing {
	class BitMatrix;
	class ResultPoint;
	class DecoderResult;
	template <typename T> class Nullable;

	namespace Pdf417 {
		/**
		* @author Guenther Grau
		*/
		class ScanningDecoder {
		public:
			static DecoderResult Decode(const BitMatrix& image,
				const Nullable<ResultPoint>& imageTopLeft, const Nullable<ResultPoint>& imageBottomLeft,
				const Nullable<ResultPoint>& imageTopRight, const Nullable<ResultPoint>& imageBottomRight,
				int minCodewordWidth, int maxCodewordWidth);
		};

		inline int NumECCodeWords(int ecLevel) { return 1 << (ecLevel + 1); }
		DecoderResult DecodeCodewords(std::vector<int>& codewords, int numECCodeWords);
	} // Pdf417
} // ZXing
//#include "pdf417/PDFWriter.h"
namespace ZXing {
	class BitMatrix;

	namespace Pdf417 {
		enum class Compaction;

		class Encoder;
		/**
		 * @author Jacob Haynes
		 * @author qwandor@google.com (Andrew Walbran)
		 */
		class Writer {
		public:
			Writer();
			Writer(Writer &&) noexcept;
			~Writer();
			Writer& setMargin(int margin) 
			{
				_margin = margin; return *this;
			}
			Writer& setErrorCorrectionLevel(int ecLevel) 
			{
				_ecLevel = ecLevel; return *this;
			}
			/**
			 * Sets max/min row/col values
			 *
			 * @param maxCols maximum allowed columns
			 * @param minCols minimum allowed columns
			 * @param maxRows maximum allowed rows
			 * @param minRows minimum allowed rows
			 */
			Writer& setDimensions(int minCols, int maxCols, int minRows, int maxRows);
			/**
			 * @param compaction compaction mode to use
			 */
			Writer& setCompaction(Compaction compaction);
			/**
			 * @param compact if true, enables compaction
			 */
			Writer& setCompact(bool compact);
			/**
			 * @param encoding sets character encoding to use
			 */
			Writer& setEncoding(CharacterSet encoding);
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _margin = -1;
			int _ecLevel = -1;
			std::unique_ptr<Encoder> _encoder;
		};
	} // Pdf417
} // ZXing
//#include "qrcode/QRReader.h"
namespace ZXing::QRCode {
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;

		Result decode(const BinaryBitmap& image) const override;
		Results decode(const BinaryBitmap& image, int maxSymbols) const override;
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRErrorCorrectionLevel.h"
namespace ZXing::QRCode {
	/**
	 * <p>See ISO 18004:2006, 6.5.1. This enum encapsulates the four error correction levels
	 * defined by the QR code standard.</p>
	 *
	 * @author Sean Owen
	 */
	enum class ErrorCorrectionLevel {
		Low,     // L = ~7 % correction
		Medium,  // M = ~15% correction
		Quality, // Q = ~25% correction
		High,    // H = ~30% correction
		Invalid, // denotes in invalid/unknown value
	};

	const char* ToString(ErrorCorrectionLevel l);
	ErrorCorrectionLevel ECLevelFromString(const char* str);
	ErrorCorrectionLevel ECLevelFromBits(int bits, const bool isMicro = false);
	int BitsFromECLevel(ErrorCorrectionLevel l);

	enum class Type {
		Model1,
		Model2,
		Micro,
		rMQR,
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRBitMatrixParser.h"
namespace ZXing {
	class BitMatrix;
	class ByteArray;

	namespace QRCode {
		class Version;
		class FormatInformation;

		/**
		 * @brief Reads version information from the QR Code.
		 * @return {@link Version} encapsulating the QR Code's version, nullptr if neither location can be parsed
		 */
		const Version* ReadVersion(const BitMatrix& bitMatrix, Type type);

		/**
		 * @brief Reads format information from one of its two locations within the QR Code.
		 * @return {@link FormatInformation} encapsulating the QR Code's format info, result is invalid if both format
		 * information locations cannot be parsed as the valid encoding of format information
		 */
		FormatInformation ReadFormatInformation(const BitMatrix& bitMatrix);

		/**
		 * @brief Reads the codewords from the BitMatrix.
		 * @return bytes encoded within the QR Code or empty array if the exact number of bytes expected is not read
		 */
		ByteArray ReadCodewords(const BitMatrix& bitMatrix, const Version& version, const FormatInformation& formatInfo);
	} // QRCode
} // ZXing
//#include "qrcode/QRDataMask.h"
namespace ZXing::QRCode {
	/**
	 * <p>Encapsulates data masks for the data bits in a QR  and micro QR code, per ISO 18004:2006 6.8.</p>
	 *
	 * <p>Note that the diagram in section 6.8.1 is misleading since it indicates that i is column position
	 * and j is row position. In fact, as the text says, i is row position and j is column position.</p>
	 */
	inline bool GetDataMaskBit(int maskIndex, int x, int y, bool isMicro = false)
	{
		if(isMicro) {
			if(maskIndex < 0 || maskIndex >= 4)
				throw std::invalid_argument("QRCode maskIndex out of range");
			maskIndex = std::array{1, 4, 6, 7}[maskIndex]; // map from MQR to QR indices
		}
		switch(maskIndex) {
			case 0: return (y + x) % 2 == 0;
			case 1: return y % 2 == 0;
			case 2: return x % 3 == 0;
			case 3: return (y + x) % 3 == 0;
			case 4: return ((y / 2) + (x / 3)) % 2 == 0;
			case 5: return (y * x) % 6 == 0;
			case 6: return ((y * x) % 6) < 3;
			case 7: return (y + x + ((y * x) % 3)) % 2 == 0;
		}
		throw std::invalid_argument("QRCode maskIndex out of range");
	}
	inline bool GetMaskedBit(const BitMatrix& bits, int x, int y, int maskIndex, bool isMicro = false)
	{
		return GetDataMaskBit(maskIndex, x, y, isMicro) != bits.get(x, y);
	}
} // namespace ZXing::QRCode
//#include "qrcode/QRFormatInformation.h"
namespace ZXing::QRCode {
	static constexpr uint32_t FORMAT_INFO_MASK_MODEL2 = 0x5412;
	static constexpr uint32_t FORMAT_INFO_MASK_MODEL1 = 0x2825;
	static constexpr uint32_t FORMAT_INFO_MASK_MICRO = 0x4445;
	static constexpr uint32_t FORMAT_INFO_MASK_RMQR = 0x1FAB2; // Finder pattern side
	static constexpr uint32_t FORMAT_INFO_MASK_RMQR_SUB = 0x20A7B; // Finder sub pattern side

	class FormatInformation {
	public:
		uint32_t mask = 0;
		uint8_t data = 255;
		uint8_t hammingDistance = 255;
		uint8_t bitsIndex = 255;

		bool isMirrored = false;
		uint8_t dataMask = 0;
		uint8_t microVersion = 0;
		uint8_t rMQRVersion = 0;
		ErrorCorrectionLevel ecLevel = ErrorCorrectionLevel::Invalid;

		FormatInformation() = default;

		static FormatInformation DecodeQR(uint32_t formatInfoBits1, uint32_t formatInfoBits2);
		static FormatInformation DecodeMQR(uint32_t formatInfoBits);
		static FormatInformation DecodeRMQR(uint32_t formatInfoBits1, uint32_t formatInfoBits2);

		// Hamming distance of the 32 masked codes is 7, by construction, so <= 3 bits differing means we found a match
		bool isValid() const { return hammingDistance <= 3; }
		Type type() const
		{
			switch(mask) {
				case FORMAT_INFO_MASK_MODEL1: return Type::Model1;
				case FORMAT_INFO_MASK_MICRO: return Type::Micro;
				default: return Type::Model2;
			}
		}
		bool operator==(const FormatInformation& other) const
		{
			return dataMask == other.dataMask && ecLevel == other.ecLevel && type() == other.type();
		}
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRECB.h"
namespace ZXing::QRCode {
	/**
	 * <p>Encapsulates the parameters for one error-correction block in one symbol version.
	 * This includes the number of data codewords, and the number of times a block with these
	 * parameters is used consecutively in the QR code version's format.</p>
	 *
	 * @author Sean Owen
	 */
	struct ECB {
		int count;
		int dataCodewords;
	};

	/**
	 * <p>Encapsulates a set of error-correction blocks in one symbol version. Most versions will
	 * use blocks of differing sizes within one version, so, this encapsulates the parameters for
	 * each set of blocks. It also holds the number of error-correction codewords per block since it
	 * will be the same across all blocks within one version.</p>
	 *
	 * @author Sean Owen
	 */
	struct ECBlocks {
		int codewordsPerBlock;
		std::array<ECB, 2> blocks;

		int numBlocks() const { return blocks[0].count + blocks[1].count; }
		int totalCodewords() const { return codewordsPerBlock * numBlocks(); }
		int totalDataCodewords() const
		{
			return blocks[0].count * (blocks[0].dataCodewords + codewordsPerBlock) + blocks[1].count * (blocks[1].dataCodewords + codewordsPerBlock);
		}
		const std::array<ECB, 2>& blockArray() const { return blocks; }
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRVersion.h"
namespace ZXing {
	class BitMatrix;

	namespace QRCode {
		/**
		 * See ISO 18004:2006 Annex D
		 */
		class Version {
		public:
			Type type() const { return _type; }
			bool isMicro() const { return type() == Type::Micro; }
			bool isRMQR() const { return type() == Type::rMQR; }
			bool isModel1() const { return type() == Type::Model1; }
			bool isModel2() const { return type() == Type::Model2; }
			int versionNumber() const { return _versionNumber; }
			const std::vector<int>& alignmentPatternCenters() const { return _alignmentPatternCenters; }
			int totalCodewords() const { return _totalCodewords; }
			int dimension() const { return DimensionOfVersion(_versionNumber, isMicro()); }
			const ECBlocks& ecBlocksForLevel(ErrorCorrectionLevel ecLevel) const { return _ecBlocks[(int)ecLevel]; }
			BitMatrix buildFunctionPattern() const;
			static constexpr int DimensionStep(bool isMicro) { return std::array{4, 2}[isMicro]; }
			static constexpr int DimensionOffset(bool isMicro) { return std::array{17, 9}[isMicro]; }
			static constexpr int DimensionOfVersion(int version, bool isMicro) { return DimensionOffset(isMicro) + DimensionStep(isMicro) * version; }
			static PointI DimensionOfVersionRMQR(int versionNumber);
			static bool HasMicroSize(const BitMatrix& bitMatrix);
			static bool HasRMQRSize(const BitMatrix& bitMatrix);
			static bool HasValidSize(const BitMatrix& bitMatrix);
			static int Number(const BitMatrix& bitMatrix);
			static const Version* DecodeVersionInformation(int versionBitsA, int versionBitsB = 0);
			static const Version* Model1(int number);
			static const Version* Model2(int number);
			static const Version* Micro(int number);
			static const Version* rMQR(int number);
		private:
			int _versionNumber;
			std::vector<int> _alignmentPatternCenters;
			std::array<ECBlocks, 4> _ecBlocks;
			int _totalCodewords;
			Type _type;

			Version(int versionNumber, std::initializer_list<int> alignmentPatternCenters, const std::array<ECBlocks, 4> &ecBlocks);
			Version(int versionNumber, const std::array<ECBlocks, 4>& ecBlocks);
		};
	} // QRCode
} // ZXing
//#include "qrcode/QRCodecMode.h"
namespace ZXing::QRCode {
	class Version;

	/**
	* <p>See ISO 18004:2006, 6.4.1, Tables 2 and 3. This enum encapsulates the various modes in which
	* data can be encoded to bits in the QR code standard.</p>
	*/
	enum class CodecMode {
		TERMINATOR           = 0x00, // Not really a mode...
		NUMERIC              = 0x01,
		ALPHANUMERIC         = 0x02,
		STRUCTURED_APPEND    = 0x03,
		BYTE                 = 0x04,
		FNC1_FIRST_POSITION  = 0x05,
		ECI                  = 0x07, // character counts don't apply
		KANJI                = 0x08,
		FNC1_SECOND_POSITION = 0x09,
		HANZI                = 0x0D, // See GBT 18284-2000; "Hanzi" is a transliteration of this mode name.
	};
	/**
	 * @param bits variable number of bits encoding a QR Code data mode
	 * @param type type of QR Code
	 * @return Mode encoded by these bits
	 * @throws FormatError if bits do not correspond to a known mode
	 */
	CodecMode CodecModeForBits(int bits, Type type);
	/**
	 * @param version version in question
	 * @return number of bits used, in this QR Code symbol {@link Version}, to encode the
	 *         count of characters that will follow encoded in this Mode
	 */
	int CharacterCountBits(CodecMode mode, const Version& version);
	/**
	 * @param version version in question
	 * @return number of bits used to encode a codec mode.
	 */
	int CodecModeBitsLength(const Version& version);

	/**
	 * @param version version in question
	 * @return number of bits in the Terminator code.
	 */
	int TerminatorBitsLength(const Version& version);
} // namespace ZXing::QRCode
//#include "qrcode/QRDataBlock.h"
namespace ZXing::QRCode {
	class Version;
	enum class ErrorCorrectionLevel;
	/**
	 * <p>Encapsulates a block of data within a QR Code. QR Codes may split their data into
	 * multiple blocks, each of which is a unit of data and error-correction codewords. Each
	 * is represented by an instance of this class.</p>
	 *
	 * @author Sean Owen
	 */
	class DataBlock {
	public:
		int numDataCodewords() const { return _numDataCodewords; }
		const ByteArray& codewords() const { return _codewords; }
		ByteArray& codewords() { return _codewords; }
		/**
		 * <p>When QR Codes use multiple data blocks, they are actually interleaved.
		 * That is, the first byte of data block 1 to n is written, then the second bytes, and so on. This
		 * method will separate the data into original blocks.</p>
		 *
		 * @param rawCodewords bytes as read directly from the QR Code
		 * @param version version of the QR Code
		 * @param ecLevel error-correction level of the QR Code
		 * @return DataBlocks containing original bytes, "de-interleaved" from representation in the
		 *         QR Code
		 */
		static std::vector<DataBlock> GetDataBlocks(const ByteArray& rawCodewords, const Version& version, ErrorCorrectionLevel ecLevel);
	private:
		int _numDataCodewords = 0;
		ByteArray _codewords;
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRDecoder.h"
namespace ZXing {
	class DecoderResult;
	class BitMatrix;

	namespace QRCode {
		DecoderResult Decode(const BitMatrix& bits);
	} // QRCode
} // ZXing
//#include "qrcode/QRDetector.h"
namespace ZXing {
	class DetectorResult;
	class BitMatrix;

	namespace QRCode {
		struct FinderPatternSet {
			ConcentricPattern bl, tl, tr;
		};

		using FinderPatterns = std::vector<ConcentricPattern>;
		using FinderPatternSets = std::vector<FinderPatternSet>;

		FinderPatterns FindFinderPatterns(const BitMatrix & image, bool tryHarder);
		FinderPatternSets GenerateFinderPatternSets(FinderPatterns & patterns);
		DetectorResult SampleQR(const BitMatrix & image, const FinderPatternSet & fp);
		DetectorResult SampleMQR(const BitMatrix & image, const ConcentricPattern & fp);
		DetectorResult SampleRMQR(const BitMatrix& image, const ConcentricPattern& fp);
		DetectorResult DetectPureQR(const BitMatrix & image);
		DetectorResult DetectPureMQR(const BitMatrix & image);
		DetectorResult DetectPureRMQR(const BitMatrix & image);
	} // QRCode
} // ZXing
//#include "qrcode/QREncoder.h"
namespace ZXing::QRCode {
    enum class ErrorCorrectionLevel;

    class EncodeResult;

    EncodeResult Encode(const std::wstring& content, ErrorCorrectionLevel ecLevel, CharacterSet encoding, int versionNumber,
        bool useGs1Format, int maskPattern = -1);
} // namespace ZXing::QRCode
//#include "qrcode/QREncodeResult.h"
namespace ZXing::QRCode {
	/**
	 * @author satorux@google.com (Satoru Takabayashi) - creator
	 * @author dswitkin@google.com (Daniel Switkin) - ported from C++
	 *
	 * Original class name in Java was QRCode, as this name is taken already for the namespace,
	 * so it's renamed here EncodeResult.
	 */
	class EncodeResult {
	public:
		ErrorCorrectionLevel ecLevel = ErrorCorrectionLevel::Invalid;
		CodecMode mode = CodecMode::TERMINATOR;
		const Version* version = nullptr;
		int maskPattern = -1;
		BitMatrix matrix;
	};
} // namespace ZXing::QRCode
//#include "qrcode/QRMaskUtil.h"
namespace ZXing::QRCode::MaskUtil {
	int CalculateMaskPenalty(const TritMatrix& matrix);
} // namespace ZXing::QRCode::MaskUtil
//#include "qrcode/QRMatrixUtil.h"
namespace ZXing {
	class BitArray;

	namespace QRCode {
		enum class ErrorCorrectionLevel;
		class Version;

		constexpr int NUM_MASK_PATTERNS = 8;

		void BuildMatrix(const BitArray& dataBits, ErrorCorrectionLevel ecLevel, const Version& version, int maskPattern, TritMatrix& matrix);
	} // QRCode
} // ZXing
//#include "qrcode/QRWriter.h"
namespace ZXing {
	class BitMatrix;

	namespace QRCode {
		enum class ErrorCorrectionLevel;
		/**
		 * This object renders a QR Code as a BitMatrix 2D array of greyscale values.
		 *
		 * @author dswitkin@google.com (Daniel Switkin)
		 */
		class Writer {
		public:
			Writer();
			Writer& setMargin(int margin) 
			{
				_margin = margin;
				return *this;
			}
			Writer& setErrorCorrectionLevel(ErrorCorrectionLevel ecLevel) 
			{
				_ecLevel = ecLevel;
				return *this;
			}
			Writer& setEncoding(CharacterSet encoding) 
			{
				_encoding = encoding;
				return *this;
			}
			Writer& setVersion(int versionNumber) 
			{
				_version = versionNumber;
				return *this;
			}
			Writer& useGS1Format() 
			{
				_useGs1Format = true;
				return *this;
			}
			Writer& setMaskPattern(int pattern) 
			{
				_maskPattern = pattern;
				return *this;
			}
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			int _margin;
			ErrorCorrectionLevel _ecLevel;
			CharacterSet _encoding;
			int _version;
			bool _useGs1Format;
			int _maskPattern;
		};
	} // QRCode
} // ZXing
//#include "aztec/AZReader.h"
namespace ZXing::Aztec {
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;

		Result decode(const BinaryBitmap& image) const override;
		Results decode(const BinaryBitmap& image, int maxSymbols) const override;
	};
} // namespace ZXing::Aztec
//#include "aztec/AZDecoder.h"
namespace ZXing {
	class DecoderResult;

	namespace Aztec {
		class DetectorResult;

		DecoderResult Decode(const DetectorResult& detectorResult);
	} // Aztec
} // ZXing
//#include "aztec/AZDetector.h"
namespace ZXing {
	class BitMatrix;
	namespace Aztec {
		class DetectorResult;
		/**
		 * Detects an Aztec Code in an image.
		 *
		 * @param isMirror if true, image is a mirror-image of original
		 */
		DetectorResult Detect(const BitMatrix& image, bool isPure, bool tryHarder = true);

		using DetectorResults = std::vector<DetectorResult>;
		DetectorResults Detect(const BitMatrix& image, bool isPure, bool tryHarder, int maxSymbols);
	} // Aztec
} // ZXing
//#include "aztec/AZDetectorResult.h"
namespace ZXing::Aztec {
	class DetectorResult : public ZXing::DetectorResult {
		bool _compact = false;
		int _nbDatablocks = 0;
		int _nbLayers = 0;
		bool _readerInit = false;
		bool _isMirrored = false;
		DetectorResult(const DetectorResult&) = delete;
		DetectorResult& operator=(const DetectorResult&) = delete;
	public:
		DetectorResult() = default;
		DetectorResult(DetectorResult&&) noexcept = default;
		DetectorResult& operator=(DetectorResult&&) noexcept = default;

		DetectorResult(ZXing::DetectorResult&& result, bool isCompact, int nbDatablocks, int nbLayers, bool readerInit, bool isMirrored)
			: ZXing::DetectorResult{std::move(result)}, _compact(isCompact), _nbDatablocks(nbDatablocks), _nbLayers(nbLayers),
			  _readerInit(readerInit), _isMirrored(isMirrored)
		{
		}
		bool isCompact() const { return _compact; }
		int nbDatablocks() const { return _nbDatablocks; }
		int nbLayers() const { return _nbLayers; }
		bool readerInit() const { return _readerInit; }
		bool isMirrored() const { return _isMirrored; }
	};
} // namespace ZXing::Aztec
//#include "aztec/AZEncoder.h"
namespace ZXing::Aztec {
	/**
	* Aztec 2D code representation
	*
	* @author Rustam Abdullaev
	*/
	struct EncodeResult {
		bool compact;
		int size;
		int layers;
		int codeWords;
		BitMatrix matrix;
	};
	/**
	* Generates Aztec 2D barcodes.
	*
	* @author Rustam Abdullaev
	*/
	class Encoder {
	public:
		static const int DEFAULT_EC_PERCENT = 33; // default minimal percentage of error check words
		static const int DEFAULT_AZTEC_LAYERS = 0;
		static EncodeResult Encode(const std::string& data, int minECCPercent, int userSpecifiedLayers);
	};
} // namespace ZXing::Aztec
//#include "aztec/AZHighLevelEncoder.h"
namespace ZXing {
	class BitArray;

	namespace Aztec {
		/**
		 * This produces nearly optimal encodings of text into the first-level of
		 * encoding used by Aztec code.
		 *
		 * It uses a dynamic algorithm.  For each prefix of the string, it determines
		 * a set of encodings that could lead to this prefix.  We repeatedly add a
		 * character and generate a new set of optimal encodings until we have read
		 * through the entire input.
		 *
		 * @author Frank Yellin
		 * @author Rustam Abdullaev
		 */
		class HighLevelEncoder {
		public:
			static BitArray Encode(const std::string& text);
		};
	} // Aztec
} // ZXing
//#include "aztec/AZToken.h"
namespace ZXing {
	class BitArray;

	namespace Aztec {
		class Token {
		public:
			void appendTo(BitArray& bitArray, const std::string& text) const;
			static Token CreateSimple(int value, int bitCount) { return {value, -bitCount}; }
			static Token CreateBinaryShift(int start, int byteCount) { return {start, byteCount}; }
		private:
			short _value;
			short _count;   // is simple token if negative,
		public:
			Token(int value, int count) : _value((short)value), _count((short)count) 
			{
			}
		};
	} // Aztec
} // ZXing
//#include "aztec/AZEncodingState.h"
namespace ZXing::Aztec {
	class Token;
	/**
	* State represents all information about a sequence necessary to generate the current output.
	* Note that a state is immutable.
	*/
	class EncodingState {
	public:
		// The list of tokens that we output.  If we are in Binary Shift mode, this
		// token list does *not* yet included the token for those bytes
		std::vector<Token> tokens;
		// The current mode of the encoding (or the mode to which we'll return if
		// we're in Binary Shift mode.
		int mode = 0;
		int binaryShiftByteCount = 0; // If non-zero, the number of most recent bytes that should be output in Binary Shift mode.
		int bitCount = 0; // The total number of bits generated (including Binary Shift).
	};
} // namespace ZXing::Aztec
//#include "aztec/AZWriter.h"
namespace ZXing {
	class BitMatrix;
	namespace Aztec {
		class Writer {
		public:
			Writer();
			Writer& setMargin(int margin) 
			{
				_margin = margin;
				return *this;
			}
			Writer& setEncoding(CharacterSet encoding) 
			{
				_encoding = encoding;
				return *this;
			}
			Writer& setEccPercent(int percent) 
			{
				_eccPercent = percent;
				return *this;
			}
			Writer& setLayers(int layers) 
			{
				_layers = layers;
				return *this;
			}
			BitMatrix encode(const std::wstring& contents, int width, int height) const;
			BitMatrix encode(const std::string& contents, int width, int height) const;
		private:
			CharacterSet _encoding;
			int _eccPercent;
			int _layers;
			int _margin = 0;
		};
	} // Aztec
} // ZXing
//#include "maxicode/MCBitMatrixParser.h"
namespace ZXing {
	class ByteArray;
	class BitMatrix;

	namespace MaxiCode {
		//
		// @author mike32767
		// @author Manuel Kasten
		//
		class BitMatrixParser {
		public:
			static ByteArray ReadCodewords(const BitMatrix& image);

			static const int MATRIX_WIDTH = 30;
			static const int MATRIX_HEIGHT = 33;
		};
	} // MaxiCode
} // ZXing
//#include "maxicode/MCDecoder.h"
namespace ZXing {
	class DecoderResult;
	class BitMatrix;

	namespace MaxiCode {
		DecoderResult Decode(const BitMatrix& bits);
	} // MaxiCode
} // ZXing
//#include "maxicode/MCReader.h"
namespace ZXing::MaxiCode {
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;
		Result decode(const BinaryBitmap& image) const override;
	};
}

#endif // __ZXING_INTERNAL_H
