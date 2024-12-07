// SLPORT.H
// Copyright (c) A.Sobolev 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
// Этот заголовочный файл призван унифицировать большинство макроопределений и деклараций, реализующих портируемость
// компиляции между платформами и компиляторами.
// Фактически, я буду здесь собирать эклектические выжимки из разных библиотек, унифицируя их как только могу.
//
// Да, я знаю, что не первый пытаюсь это сделать, и предполагаю что выйдет плохо, но меня замучали дублированные определения,
// и невозможность скомпилировать сторонние библиотеки, которые мне нужны.
//
#ifndef __SLPORT_H // {
#define __SLPORT_H
//#include <cxx_detect.h> // @v10.9.0
	//
	// CXX_DETECT - C++ Compiler and Features Detection
	// Dual licensed under PUBLIC DOMAIN (UNLICENSE) and ZLIB. (choose the one that you prefer and is valid in your country)
	//
	#if defined(CXX_DETECT_H)
		#undef CXX_DETECT_H
		#define CXX_DETECT_H_CLEANUP
	#endif
	#define CXX_DETECT_H 20171018
	//
	// [Cleanup]
	//
	#if defined(CXX_DETECT_H_CLEANUP)
		#undef CXX_DETECT_H_CLEANUP
		// C/C++ Compiler vendor and vendor details
		#undef CXX_CLANG
		#undef CXX_GNU
		#undef CXX_GNU_COMPAT
		#undef CXX_INTEL
		#undef CXX_MSC
		#undef CXX_MINGW
		#undef CXX_MAKE_VER

		// C++ standard version.
		#undef CXX_CPLUSPLUS

		// C++ features (X == vendor-specific).
		#undef CXX_HAS_X_ASSUME
		#undef CXX_HAS_X_ASSUME_ALIGNED
		#undef CXX_HAS_X_ATTRIBUTE
		#undef CXX_HAS_X_ATTRIBUTE_ALIGNED
		#undef CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE
		#undef CXX_HAS_X_ATTRIBUTE_NOINLINE
		#undef CXX_HAS_X_ATTRIBUTE_DEPRECATED
		#undef CXX_HAS_X_ATTRIBUTE_NORETURN
		#undef CXX_HAS_X_ATTRIBUTE_NOSANITIZE
		#undef CXX_HAS_X_ATTRIBUTE_OPTIMIZE
		#undef CXX_HAS_X_BUILTIN_ASSUME
		#undef CXX_HAS_X_BUILTIN_ASSUME_ALIGNED
		#undef CXX_HAS_X_BUILTIN_EXPECT
		#undef CXX_HAS_X_BUILTIN_UNREACHABLE
		#undef CXX_HAS_X_DECLSPEC_ALIGN
		#undef CXX_HAS_X_DECLSPEC_DEPRECATED
		#undef CXX_HAS_X_DECLSPEC_NOINLINE
		#undef CXX_HAS_X_DECLSPEC_NORETURN
		#undef CXX_HAS_X_FORCEINLINE

		// C++ features (C++11).
		#undef CXX_HAS_ALIAS_TEMPLATES
		#undef CXX_HAS_ALIGNAS
		#undef CXX_HAS_ALIGNOF
		#undef CXX_HAS_ATTRIBUTES
		#undef CXX_HAS_AUTO_TYPE
		#undef CXX_HAS_CONSTEXPR
		#undef CXX_HAS_DECLTYPE
		#undef CXX_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGS
		#undef CXX_HAS_DEFAULTED_FUNCTIONS
		#undef CXX_HAS_DELEGATING_CONSTRUCTORS
		#undef CXX_HAS_DELETED_FUNCTIONS
		#undef CXX_HAS_EXPLICIT_CONVERSIONS
		#undef CXX_HAS_FINAL
		#undef CXX_HAS_INHERITING_CONSTRUCTORS
		#undef CXX_HAS_INITIALIZER_LISTS
		#undef CXX_HAS_INLINE_NAMESPACES
		#undef CXX_HAS_LAMBDAS
		#undef CXX_HAS_LOCAL_TYPE_TEMPLATE_ARGS
		#undef CXX_HAS_NOEXCEPT
		#undef CXX_HAS_NONSTATIC_MEMBER_INIT
		#undef CXX_HAS_NULLPTR
		#undef CXX_HAS_OVERRIDE
		#undef CXX_HAS_RANGE_FOR
		#undef CXX_HAS_RAW_STRING_LITERALS
		#undef CXX_HAS_REFERENCE_QUALIFIED_FUNCTIONS
		#undef CXX_HAS_RVALUE_REFERENCES
		#undef CXX_HAS_STATIC_ASSERT
		#undef CXX_HAS_STRONG_ENUMS
		#undef CXX_HAS_THREAD_LOCAL
		#undef CXX_HAS_UNICODE_LITERALS
		#undef CXX_HAS_UNRESTRICTED_UNIONS
		#undef CXX_HAS_VARIADIC_TEMPLATES

		// C++ features (C++14).
		#undef CXX_HAS_AGGREGATE_NSDMI
		#undef CXX_HAS_BINARY_LITERALS
		#undef CXX_HAS_CONTEXTUAL_CONVERSIONS
		#undef CXX_HAS_DECLTYPE_AUTO
		#undef CXX_HAS_GENERIC_LAMBDAS
		#undef CXX_HAS_INIT_CAPTURES
		#undef CXX_HAS_RELAXED_CONSTEXPR
		#undef CXX_HAS_VARIABLE_TEMPLATES

		// C++ features (C++17).

		// C++ native types.
		#undef CXX_HAS_NATIVE_CHAR
		#undef CXX_HAS_NATIVE_CHAR16_T
		#undef CXX_HAS_NATIVE_CHAR32_T
		#undef CXX_HAS_NATIVE_WCHAR_T

		// Target architecture.
		#undef CXX_ARCH_X86
		#undef CXX_ARCH_X86_64
		#undef CXX_ARCH_ARM32
		#undef CXX_ARCH_ARM64
		#undef CXX_ARCH_BITS
		#undef CXX_ARCH_LE
		#undef CXX_ARCH_BE

		// Target operating system.
		#undef CXX_OS_WINDOWS
		#undef CXX_OS_MAC
		#undef CXX_OS_IOS
		#undef CXX_OS_ANDROID
		#undef CXX_OS_LINUX
		#undef CXX_OS_NETBSD
		#undef CXX_OS_FREEBSD
		#undef CXX_OS_OPENBSD
		#undef CXX_OS_DRAGONFLYBSD
		#undef CXX_OS_QNX
		#undef CXX_OS_SOLARIS
		#undef CXX_OS_CYGWIN
		#undef CXX_OS_BSD

		// Visibility.
		#undef CXX_EXPORT
		#undef CXX_IMPORT

		// Function Attributes.
		#undef CDECL
		#undef STDCALL
		#undef FASTCALL
		#undef CXX_REGPARM
		#undef CXX_FORCEINLINE
		#undef CXX_NOINLINE
		#undef CXX_NORETURN

		// Likely / Unlikely.
		#undef CXX_LIKELY
		#undef CXX_UNLIKELY

		// Assumptions.
		#undef CXX_ASSUME
		#undef CXX_ASSUME_ALIGNED

		// Annotations.
		#undef CXX_UNUSED
		#undef CXX_FALLTHROUGH

		// Other macros.
		#undef CXX_ALIGN_DECL
		#undef CXX_ALIGN_TYPE

		//@sobolev #undef CXX_ARRAY_SIZE
		#undef CXX_OFFSET_OF
	#endif
	//
	// [C++ Compiler Vendor]
	//
	#define CXX_CLANG    0
	#define CXX_GNU      0
	#define CXX_INTEL    0
	#define CXX_MSC      0
	#define CXX_BORLANDC 0 // @sobolev @construction
	#define CXX_WATCOM   0 // @sobolev @construction

	#define CXX_MAKE_VER(MAJOR, MINOR, PATCH) ((MAJOR) * 10000000 + (MINOR) * 100000 + (PATCH))

	// Intel pretends to be GNU/MSC, so check it first.
	#if defined(__INTEL_COMPILER)
		#undef CXX_INTEL
		#define CXX_INTEL CXX_MAKE_VER(__INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) % 10, __INTEL_COMPILER % 10)
	#elif defined(_MSC_VER) && defined(_MSC_FULL_VER)
		#undef CXX_MSC
		#if _MSC_VER == _MSC_FULL_VER / 10000
			#define CXX_MSC CXX_MAKE_VER(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000)
		#else
			#define CXX_MSC CXX_MAKE_VER(_MSC_VER / 100, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000)
		#endif
	#elif defined(__clang__) && defined(__clang_minor__)
		#undef CXX_CLANG
		#define CXX_CLANG CXX_MAKE_VER(__clang_major__, __clang_minor__, __clang_patchlevel__)
	#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
		#undef CXX_GNU
		#define CXX_GNU CXX_MAKE_VER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
	#else
		#error "[ccx_detect.h] Unable to detect C/C++ compiler."
	#endif
	// CXX_MINGW - MINGW - 0 (no MINGW), 32 (MINGW32), 64 (MINGW64).
	#if defined(__MINGW64__)
		#define CXX_MINGW 64
	#elif defined(__MINGW32__)
		#define CXX_MINGW 32
	#else
		#define CXX_MINGW 0
	#endif
	// CXX_GNU_COMPAT - C++ compiler is GNU or pretends to be (clang/ICC).
	#if defined(__GNUC__) && !defined(__GNUC_MINOR__)
		#define CXX_GNU_COMPAT CXX_MAKE_VER(__GNUC__, 0, 0)
	#elif defined(__GNUC__) && !defined(__GNUC_PATCHLEVEL__)
		#define CXX_GNU_COMPAT CXX_MAKE_VER(__GNUC__, __GNUC_MINOR__, 0)
	#elif defined(__GNUC__)
		#define CXX_GNU_COMPAT CXX_MAKE_VER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
	#else
		#define CXX_GNU_COMPAT 0
	#endif
	//
	// [C++ Version]
	//
	// 199711L - C++98
	// 201103L - C++11
	// 201402L - C++14
	// 201703L - C++17
	#if defined(__cplusplus)
		#if __cplusplus >= 201103L
			#define CXX_CPLUSPLUS __cplusplus
		#elif defined(__GXX_EXPERIMENTAL_CXX0X__) || CXX_MSC >= CXX_MAKE_VER(18, 0, 0) || CXX_INTEL >= CXX_MAKE_VER(14, 0, 0)
			#define CXX_CPLUSPLUS 201103L
		#else
			#define CXX_CPLUSPLUS 199711L
		#endif
	#endif
	#if !defined(CXX_CPLUSPLUS)
		#define CXX_CPLUSPLUS 0
	#endif
	#if !defined(__has_builtin)
		#define __has_builtin(x) 0
	#endif
	//
	// [C++ Compiler Features]
	//
	// Feature Comparison: http://en.cppreference.com/w/cpp/compiler_support
	//
	// Clang Compiler: https://releases.llvm.org/5.0.0/tools/clang/docs/LanguageExtensions.html
	//
	#if CXX_CLANG
		#define CXX_HAS_X_ASSUME                       (0)
		#define CXX_HAS_X_ASSUME_ALIGNED               (0)
		#define CXX_HAS_X_ATTRIBUTE                    (1)
		#define CXX_HAS_X_ATTRIBUTE_ALIGNED            (__has_attribute(__aligned__))
		#define CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE      (__has_attribute(__always_inline__))
		#define CXX_HAS_X_ATTRIBUTE_DEPRECATED         (__has_attribute(__deprecated__) && __has_extension(__attribute_deprecated_with_message__))
		#define CXX_HAS_X_ATTRIBUTE_NOINLINE           (__has_attribute(__noinline__))
		#define CXX_HAS_X_ATTRIBUTE_NORETURN           (__has_attribute(__noreturn__))
		#define CXX_HAS_X_ATTRIBUTE_NOSANITIZE         (__has_attribute(__nosanitize__))
		#define CXX_HAS_X_ATTRIBUTE_OPTIMIZE           (__has_attribute(__optimize__))
		#define CXX_HAS_X_ATTRIBUTE_PURE               (1) // @v11.9.2 @fixme (я не уверен, что это - верно)
		#define CXX_HAS_X_ATTRIBUTE_CONST              (0) // @v11.9.2 @fixme (я не уверен, что это - верно)
		#define CXX_HAS_X_BUILTIN_ASSUME               (__has_builtin(__builtin_assume))
		#define CXX_HAS_X_BUILTIN_ASSUME_ALIGNED       (__has_builtin(__builtin_assume_aligned))
		#define CXX_HAS_X_BUILTIN_EXPECT               (__has_builtin(__builtin_expect))
		#define CXX_HAS_X_BUILTIN_UNREACHABLE          (__has_builtin(__builtin_unreachable))
		#define CXX_HAS_X_DECLSPEC_ALIGN               (0)
		#define CXX_HAS_X_DECLSPEC_DEPRECATED          (0)
		#define CXX_HAS_X_DECLSPEC_NOINLINE            (0)
		#define CXX_HAS_X_DECLSPEC_NORETURN            (0)
		#define CXX_HAS_X_FORCEINLINE                  (0)

		#define CXX_HAS_ALIAS_TEMPLATES                (__has_extension(__cxx_alias_templates__))
		#define CXX_HAS_ALIGNAS                        (__has_extension(__cxx_alignas__))
		#define CXX_HAS_ALIGNOF                        (__has_extension(__cxx_alignof__))
		#define CXX_HAS_ATTRIBUTES                     (__has_extension(__cxx_attributes__))
		#define CXX_HAS_AUTO_TYPE                      (__has_extension(__cxx_auto_type__))
		#define CXX_HAS_CONSTEXPR                      (__has_extension(__cxx_constexpr__))
		#define CXX_HAS_DECLTYPE                       (__has_extension(__cxx_decltype__))
		#define CXX_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGS (__has_extension(__cxx_default_function_template_args__))
		#define CXX_HAS_DEFAULTED_FUNCTIONS            (__has_extension(__cxx_defaulted_functions__))
		#define CXX_HAS_DELEGATING_CONSTRUCTORS        (__has_extension(__cxx_delegating_constructors__))
		#define CXX_HAS_DELETED_FUNCTIONS              (__has_extension(__cxx_deleted_functions__))
		#define CXX_HAS_EXPLICIT_CONVERSIONS           (__has_feature(__cxx_explicit_conversions__))
		#define CXX_HAS_FINAL                          (__has_extension(__cxx_override_control__))
		#define CXX_HAS_INHERITING_CONSTRUCTORS        (__has_feature(__cxx_inheriting_constructors__))
		#define CXX_HAS_INITIALIZER_LISTS              (__has_extension(__cxx_generalized_initializers__))
		#define CXX_HAS_INLINE_NAMESPACES              (__has_extension(__cxx_inline_namespaces__))
		#define CXX_HAS_LAMBDAS                        (__has_extension(__cxx_lambdas__))
		#define CXX_HAS_LOCAL_TYPE_TEMPLATE_ARGS       (__has_extension(__cxx_local_type_template_args__))
		#define CXX_HAS_NOEXCEPT                       (__has_extension(__cxx_noexcept__))
		#define CXX_HAS_NONSTATIC_MEMBER_INIT          (__has_feature(__cxx_nonstatic_member_init__))
		#define CXX_HAS_NULLPTR                        (__has_extension(__cxx_nullptr__))
		#define CXX_HAS_OVERRIDE                       (__has_extension(__cxx_override_control__))
		#define CXX_HAS_RANGE_FOR                      (__has_extension(__cxx_range_for__))
		#define CXX_HAS_RAW_STRING_LITERALS            (__has_feature(__cxx_raw_string_literals__))
		#define CXX_HAS_REFERENCE_QUALIFIED_FUNCTIONS  (__has_extension(__cxx_reference_qualified_functions__))
		#define CXX_HAS_RVALUE_REFERENCES              (__has_extension(__cxx_rvalue_references__))
		#define CXX_HAS_STATIC_ASSERT                  (__has_extension(__cxx_static_assert__))
		#define CXX_HAS_STRONG_ENUMS                   (__has_extension(__cxx_strong_enums__))
		#define CXX_HAS_THREAD_LOCAL                   (__has_feature(__cxx_thread_local__))
		#define CXX_HAS_UNICODE_LITERALS               (__has_extension(__cxx_unicode_literals__))
		#define CXX_HAS_UNRESTRICTED_UNIONS            (__has_feature(__cxx_unrestricted_unions__))
		#define CXX_HAS_VARIADIC_TEMPLATES             (__has_extension(__cxx_variadic_templates__))

		#define CXX_HAS_AGGREGATE_NSDMI                (__has_extension(__cxx_aggregate_nsdmi__))
		#define CXX_HAS_BINARY_LITERALS                (__has_extension(__cxx_binary_literals__))
		#define CXX_HAS_CONTEXTUAL_CONVERSIONS         (__has_extension(__cxx_contextual_conversions__))
		#define CXX_HAS_DECLTYPE_AUTO                  (__has_extension(__cxx_decltype_auto__))
		#define CXX_HAS_GENERIC_LAMBDAS                (__has_extension(__cxx_generic_lambdas__))
		#define CXX_HAS_INIT_CAPTURES                  (__has_extension(__cxx_init_captures__))
		#define CXX_HAS_RELAXED_CONSTEXPR              (__has_extension(__cxx_relaxed_constexpr__))
		#define CXX_HAS_VARIABLE_TEMPLATES             (__has_extension(__cxx_variable_templates__))

		#define CXX_HAS_NATIVE_CHAR                    (1)
		#define CXX_HAS_NATIVE_CHAR16_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_CHAR32_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_WCHAR_T                 (1)
	#endif
	//
	// GNU Compiler: https://gcc.gnu.org/projects/cxx-status.html
	//
	#if CXX_GNU
		#define CXX_HAS_X_ASSUME                       (0)
		#define CXX_HAS_X_ASSUME_ALIGNED               (0)
		#define CXX_HAS_X_ATTRIBUTE                    (1)
		#define CXX_HAS_X_ATTRIBUTE_ALIGNED            (CXX_GNU >= CXX_MAKE_VER(2, 7, 0))
		#define CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE      (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && !CXX_MINGW)
		#define CXX_HAS_X_ATTRIBUTE_DEPRECATED         (CXX_GNU >= CXX_MAKE_VER(4, 5, 0))
		#define CXX_HAS_X_ATTRIBUTE_NOINLINE           (CXX_GNU >= CXX_MAKE_VER(3, 4, 0) && !CXX_MINGW)
		#define CXX_HAS_X_ATTRIBUTE_NORETURN           (CXX_GNU >= CXX_MAKE_VER(2, 5, 0))
		#define CXX_HAS_X_ATTRIBUTE_NOSANITIZE         (0)
		#define CXX_HAS_X_ATTRIBUTE_OPTIMIZE           (CXX_GNU >= CXX_MAKE_VER(4, 4, 0))
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDRESULT       (CXX_GNU >= CXX_MAKE_VER(2, 0, 7)) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDPARAM        (CXX_GNU >= CXX_MAKE_VER(2, 0, 7)) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_PURE               (CXX_GNU >= CXX_MAKE_VER(3, 0, 0)) // @v11.9.2
		#define CXX_HAS_X_ATTRIBUTE_CONST              (CXX_GNU >= CXX_MAKE_VER(3, 0, 0)) // @v11.9.2
		#define CXX_HAS_X_BUILTIN_ASSUME               (0)
		#define CXX_HAS_X_BUILTIN_ASSUME_ALIGNED       (CXX_GNU >= CXX_MAKE_VER(4, 7, 0))
		#define CXX_HAS_X_BUILTIN_EXPECT               (1)
		#define CXX_HAS_X_BUILTIN_UNREACHABLE          (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_X_DECLSPEC_ALIGN               (0)
		#define CXX_HAS_X_DECLSPEC_DEPRECATED          (0)
		#define CXX_HAS_X_DECLSPEC_NOINLINE            (0)
		#define CXX_HAS_X_DECLSPEC_NORETURN            (0)
		#define CXX_HAS_X_FORCEINLINE                  (0)

		#define CXX_HAS_ALIAS_TEMPLATES                (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_ALIGNAS                        (CXX_GNU >= CXX_MAKE_VER(4, 8, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_ALIGNOF                        (CXX_GNU >= CXX_MAKE_VER(4, 8, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_ATTRIBUTES                     (CXX_GNU >= CXX_MAKE_VER(4, 8, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_AUTO_TYPE                      (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_CONSTEXPR                      (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_DECLTYPE                       (CXX_GNU >= CXX_MAKE_VER(4, 3, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGS (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_DEFAULTED_FUNCTIONS            (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_DELEGATING_CONSTRUCTORS        (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_DELETED_FUNCTIONS              (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_EXPLICIT_CONVERSIONS           (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_FINAL                          (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_INHERITING_CONSTRUCTORS        (CXX_GNU >= CXX_MAKE_VER(4, 8, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_INITIALIZER_LISTS              (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_INLINE_NAMESPACES              (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_LAMBDAS                        (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_LOCAL_TYPE_TEMPLATE_ARGS       (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_NOEXCEPT                       (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_NONSTATIC_MEMBER_INIT          (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_NULLPTR                        (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_OVERRIDE                       (CXX_GNU >= CXX_MAKE_VER(4, 7, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_RANGE_FOR                      (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_RAW_STRING_LITERALS            (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_REFERENCE_QUALIFIED_FUNCTIONS  (CXX_GNU >= CXX_MAKE_VER(4, 8, 1) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_RVALUE_REFERENCES              (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_STATIC_ASSERT                  (CXX_GNU >= CXX_MAKE_VER(4, 3, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_STRONG_ENUMS                   (CXX_GNU >= CXX_MAKE_VER(4, 4, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_THREAD_LOCAL                   (CXX_GNU >= CXX_MAKE_VER(4, 8, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_UNICODE_LITERALS               (CXX_GNU >= CXX_MAKE_VER(4, 5, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_UNRESTRICTED_UNIONS            (CXX_GNU >= CXX_MAKE_VER(4, 6, 0) && CXX_CPLUSPLUS >= 201103L)
		#define CXX_HAS_VARIADIC_TEMPLATES             (CXX_GNU >= CXX_MAKE_VER(4, 3, 0) && CXX_CPLUSPLUS >= 201103L)

		#define CXX_HAS_AGGREGATE_NSDMI                (CXX_GNU >= CXX_MAKE_VER(5, 0, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_BINARY_LITERALS                (CXX_GNU >= CXX_MAKE_VER(4, 9, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_CONTEXTUAL_CONVERSIONS         (CXX_GNU >= CXX_MAKE_VER(4, 9, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_DECLTYPE_AUTO                  (CXX_GNU >= CXX_MAKE_VER(4, 9, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_GENERIC_LAMBDAS                (CXX_GNU >= CXX_MAKE_VER(4, 9, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_INIT_CAPTURES                  (CXX_GNU >= CXX_MAKE_VER(4, 9, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_RELAXED_CONSTEXPR              (CXX_GNU >= CXX_MAKE_VER(5, 0, 0) && CXX_CPLUSPLUS >= 201402L)
		#define CXX_HAS_VARIABLE_TEMPLATES             (CXX_GNU >= CXX_MAKE_VER(5, 0, 0) && CXX_CPLUSPLUS >= 201402L)

		#define CXX_HAS_NATIVE_CHAR                    (1)
		#define CXX_HAS_NATIVE_CHAR16_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_CHAR32_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_WCHAR_T                 (1)
	#endif
	//
	// Intel Compiler:
	//   https://software.intel.com/en-us/articles/c0x-features-supported-by-intel-c-compiler
	//   https://software.intel.com/en-us/articles/c14-features-supported-by-intel-c-compiler
	//   https://software.intel.com/en-us/articles/c17-features-supported-by-intel-c-compiler
	#if CXX_INTEL
		#define CXX_HAS_X_ASSUME                       (1)
		#define CXX_HAS_X_ASSUME_ALIGNED               (1)
		#define CXX_HAS_X_ATTRIBUTE                    (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_ALIGNED            (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE      (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_DEPRECATED         (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_NOINLINE           (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_NORETURN           (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_NOSANITIZE         (0)
		#define CXX_HAS_X_ATTRIBUTE_OPTIMIZE           (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDRESULT       (0) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDPARAM        (0) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_PURE               (0) // @v11.9.2
		#define CXX_HAS_X_ATTRIBUTE_CONST              (0) // @v11.9.2
		#define CXX_HAS_X_BUILTIN_ASSUME               (0)
		#define CXX_HAS_X_BUILTIN_ASSUME_ALIGNED       (0)
		#define CXX_HAS_X_BUILTIN_EXPECT               (CXX_GNU_COMPAT >= 1)
		#define CXX_HAS_X_BUILTIN_UNREACHABLE          (0)
		#define CXX_HAS_X_DECLSPEC_ALIGN               (CXX_GNU_COMPAT == 0)
		#define CXX_HAS_X_DECLSPEC_DEPRECATED          (CXX_GNU_COMPAT == 0)
		#define CXX_HAS_X_DECLSPEC_NOINLINE            (CXX_GNU_COMPAT == 0)
		#define CXX_HAS_X_DECLSPEC_NORETURN            (CXX_GNU_COMPAT == 0)
		#define CXX_HAS_X_FORCEINLINE                  (CXX_GNU_COMPAT == 0)

		#define CXX_HAS_ALIAS_TEMPLATES                (CXX_INTEL >= CXX_MAKE_VER(12, 1, 0))
		#define CXX_HAS_ALIGNAS                        (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_ALIGNOF                        (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_ATTRIBUTES                     (CXX_INTEL >= CXX_MAKE_VER(12, 1, 0))
		#define CXX_HAS_AUTO_TYPE                      (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_CONSTEXPR                      (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_DECLTYPE                       (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGS (CXX_INTEL >= CXX_MAKE_VER(13, 0, 0))
		#define CXX_HAS_DEFAULTED_FUNCTIONS            (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_DELEGATING_CONSTRUCTORS        (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_DELETED_FUNCTIONS              (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_EXPLICIT_CONVERSIONS           (CXX_INTEL >= CXX_MAKE_VER(13, 0, 0))
		#define CXX_HAS_FINAL                          (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_INHERITING_CONSTRUCTORS        (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_INITIALIZER_LISTS              (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_INLINE_NAMESPACES              (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_LAMBDAS                        (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_LOCAL_TYPE_TEMPLATE_ARGS       (CXX_INTEL >= CXX_MAKE_VER(12, 0, 0))
		#define CXX_HAS_NOEXCEPT                       (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_NONSTATIC_MEMBER_INIT          (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_NULLPTR                        (CXX_INTEL >= CXX_MAKE_VER(12, 6, 0))
		#define CXX_HAS_OVERRIDE                       (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_RANGE_FOR                      (CXX_INTEL >= CXX_MAKE_VER(13, 0, 0))
		#define CXX_HAS_RAW_STRING_LITERALS            (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_REFERENCE_QUALIFIED_FUNCTIONS  (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_RVALUE_REFERENCES              (CXX_INTEL >= CXX_MAKE_VER(11, 1, 0))
		#define CXX_HAS_STATIC_ASSERT                  (CXX_INTEL >= CXX_MAKE_VER(11, 1, 0))
		#define CXX_HAS_STRONG_ENUMS                   (CXX_INTEL >= CXX_MAKE_VER(13, 0, 0))
		#define CXX_HAS_THREAD_LOCAL                   (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_UNICODE_LITERALS               (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0) || (CXX_GNU_COMPAT > 0 && CXX_INTEL >= CXX_MAKE_VER(12, 6, 0)))
		#define CXX_HAS_UNRESTRICTED_UNIONS            (CXX_INTEL >= CXX_MAKE_VER(14, 0, 0) && CXX_GNU_COMPAT)
		#define CXX_HAS_VARIADIC_TEMPLATES             (CXX_INTEL >= CXX_MAKE_VER(12, 6, 0))

		#define CXX_HAS_AGGREGATE_NSDMI                (CXX_INTEL >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_BINARY_LITERALS                (CXX_INTEL >= CXX_MAKE_VER(11, 0, 0))
		#define CXX_HAS_CONTEXTUAL_CONVERSIONS         (CXX_INTEL >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_DECLTYPE_AUTO                  (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_GENERIC_LAMBDAS                (CXX_INTEL >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_INIT_CAPTURES                  (CXX_INTEL >= CXX_MAKE_VER(15, 0, 0))
		#define CXX_HAS_RELAXED_CONSTEXPR              (CXX_INTEL >= CXX_MAKE_VER(17, 0, 0))
		#define CXX_HAS_VARIABLE_TEMPLATES             (CXX_INTEL >= CXX_MAKE_VER(17, 0, 0))

		#define CXX_HAS_NATIVE_CHAR                    (1)
		#define CXX_HAS_NATIVE_CHAR16_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_CHAR32_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_WCHAR_T                 (1)
	#endif
	//
	// MSC Compiler: https://msdn.microsoft.com/en-us/library/hh567368.aspx
	//
	// Version List:
	//   16.00.0 == VS2010
	//   17.00.0 == VS2012
	//   18.00.0 == VS2013
	//   19.00.0 == VS2015
	//   19.10.0 == VS2017
	#if CXX_MSC
		#define CXX_HAS_X_ASSUME                       (1)
		#define CXX_HAS_X_ASSUME_ALIGNED               (0)
		#define CXX_HAS_X_ATTRIBUTE                    (0)
		#define CXX_HAS_X_ATTRIBUTE_ALIGNED            (0)
		#define CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE      (0)
		#define CXX_HAS_X_ATTRIBUTE_DEPRECATED         (0)
		#define CXX_HAS_X_ATTRIBUTE_NOINLINE           (0)
		#define CXX_HAS_X_ATTRIBUTE_NORETURN           (0)
		#define CXX_HAS_X_ATTRIBUTE_NOSANITIZE         (0)
		#define CXX_HAS_X_ATTRIBUTE_OPTIMIZE           (0)
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDRESULT       (0) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_UNUSEDPARAM        (0) // @v10.9.3
		#define CXX_HAS_X_ATTRIBUTE_PURE               (0) // @v11.9.2
		#define CXX_HAS_X_ATTRIBUTE_CONST              (0) // @v11.9.2
		#define CXX_HAS_X_BUILTIN_ASSUME               (0)
		#define CXX_HAS_X_BUILTIN_ASSUME_ALIGNED       (0)
		#define CXX_HAS_X_BUILTIN_EXPECT               (0)
		#define CXX_HAS_X_BUILTIN_UNREACHABLE          (0)
		#define CXX_HAS_X_DECLSPEC_ALIGN               (1)
		#define CXX_HAS_X_DECLSPEC_DEPRECATED          (1)
		#define CXX_HAS_X_DECLSPEC_NOINLINE            (1)
		#define CXX_HAS_X_DECLSPEC_NORETURN            (1)
		#define CXX_HAS_X_FORCEINLINE                  (1)

		#define CXX_HAS_ALIAS_TEMPLATES                (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_ALIGNAS                        (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_ALIGNOF                        (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_ATTRIBUTES                     (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_AUTO_TYPE                      (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_CONSTEXPR                      (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_DECLTYPE                       (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGS (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_DEFAULTED_FUNCTIONS            (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_DELEGATING_CONSTRUCTORS        (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_DELETED_FUNCTIONS              (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_EXPLICIT_CONVERSIONS           (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_FINAL                          (CXX_MSC >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_INHERITING_CONSTRUCTORS        (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_INITIALIZER_LISTS              (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_INLINE_NAMESPACES              (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_LAMBDAS                        (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_LOCAL_TYPE_TEMPLATE_ARGS       (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_NOEXCEPT                       (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_NONSTATIC_MEMBER_INIT          (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_NULLPTR                        (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_OVERRIDE                       (CXX_MSC >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_RANGE_FOR                      (CXX_MSC >= CXX_MAKE_VER(17, 0, 0))
		#define CXX_HAS_RAW_STRING_LITERALS            (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_REFERENCE_QUALIFIED_FUNCTIONS  (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_RVALUE_REFERENCES              (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_STATIC_ASSERT                  (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		#define CXX_HAS_STRONG_ENUMS                   (CXX_MSC >= CXX_MAKE_VER(14, 0, 0))
		#define CXX_HAS_THREAD_LOCAL                   (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_UNICODE_LITERALS               (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_UNRESTRICTED_UNIONS            (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_VARIADIC_TEMPLATES             (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))

		#define CXX_HAS_AGGREGATE_NSDMI                (CXX_MSC >= CXX_MAKE_VER(19, 10, 0))
		#define CXX_HAS_BINARY_LITERALS                (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_CONTEXTUAL_CONVERSIONS         (CXX_MSC >= CXX_MAKE_VER(18, 0, 0))
		#define CXX_HAS_DECLTYPE_AUTO                  (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_GENERIC_LAMBDAS                (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_INIT_CAPTURES                  (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))
		#define CXX_HAS_RELAXED_CONSTEXPR              (CXX_MSC >= CXX_MAKE_VER(19, 10, 0))
		#define CXX_HAS_VARIABLE_TEMPLATES             (CXX_MSC >= CXX_MAKE_VER(19, 0, 0))

		#define CXX_HAS_NATIVE_CHAR                    (1)
		#define CXX_HAS_NATIVE_CHAR16_T                (CXX_HAS_UNICODE_LITERALS)
		#define CXX_HAS_NATIVE_CHAR32_T                (CXX_HAS_UNICODE_LITERALS)
		#if defined(_NATIVE_WCHAR_T_DEFINED)
			#define CXX_HAS_NATIVE_WCHAR_T                (1)
		#else
			#define CXX_HAS_NATIVE_WCHAR_T                (0)
		#endif
	#endif
	//
	// [Target Architecture]
	//
	#if(defined(_M_X64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) || defined(__amd64) || defined(__amd64__))
		#define CXX_ARCH_X86_64     (1)
	#else
		#define CXX_ARCH_X86_64     (0)
	#endif
	#if(defined(_M_IX86 ) || defined(__X86__ ) || defined(__i386) || defined(__IA32__) || defined(__I86__) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__THW_INTEL__))
		#define CXX_ARCH_X86        (!CXX_ARCH_X86_64)
	#else
		#define CXX_ARCH_X86        (0)
	#endif
	#if defined(__aarch64__)
		#define CXX_ARCH_ARM64      (1)
	#else
		#define CXX_ARCH_ARM64      (0)
	#endif
	#if(defined(_M_ARM) || defined(__arm) || defined(__thumb__) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb2__))
		#define CXX_ARCH_ARM32      (!CXX_ARCH_ARM64)
	#else
		#define CXX_ARCH_ARM32      (0)
	#endif
	#if CXX_ARCH_X86_64 || CXX_ARCH_ARM64
		#define CXX_ARCH_BITS       (64)
	#else
		#define CXX_ARCH_BITS       (32)
	#endif
	#define CXX_ARCH_LE          (CXX_ARCH_X86 || CXX_ARCH_X86_64 || CXX_ARCH_ARM32 || CXX_ARCH_ARM64)
	#define CXX_ARCH_BE          (!(CXX_ARCH_LE))
	// @v11.4.0 {
	// Следующая конструкция с изменениями позаимствована у библиотеки abseil
	// Checks the endianness of the platform.
	//
	// Notes: uses the built in endian macros provided by GCC (since 4.6) and
	// Clang (since 3.2); see
	// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html.
	// Otherwise, if _WIN32, assume little endian. Otherwise, bail with an error.
	#if defined(SL_BIGENDIAN)
		#error "SL_BIGENDIAN cannot be directly set"
	#endif
	#if defined(SL_LITTLEENDIAN)
		#error "SL_LITTLEENDIAN cannot be directly set"
	#endif
	#if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		#define SL_LITTLEENDIAN 1
	#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define SL_BIGENDIAN    1
	#elif defined(_M_PPC) // @v11.7.4 (from libwebp)
		#define SL_BIGENDIAN    1
	#elif defined(_WIN32)
		#define SL_LITTLEENDIAN 1
	#else
		#error "Endian detection needs to be set up for your compiler"
	#endif
	// Здесь инициируем проверки ради избежания конфликтов в определениях CXX_ARCH_LE, CXX_ARCH_BE, SL_LITTLEENDIAN, SL_BIGENDIAN
	#if CXX_ARCH_LE && CXX_ARCH_BE
		#error "Conflict at CXX_ARCH_LE and CXX_ARCH_BE definitions"
	#elif !defined(CXX_ARCH_LE) && !defined(CXX_ARCH_LE)
		#error "Neither CXX_ARCH_LE nor CXX_ARCH_LE defined"
	#elif defined(SL_BIGENDIAN) && defined(SL_LITTLEENDIAN)
		#error "Conflict at SL_LITTLEENDIAN and SL_BIGENDIAN definitions"
	#elif !defined(SL_BIGENDIAN) && !defined(SL_LITTLEENDIAN)
		#error "Neither SL_LITTLEENDIAN nor SL_BIGENDIAN defined"
	#elif defined(SL_BIGENDIAN) && SL_BIGENDIAN != 1
		#error "Invalid SL_BIGENDIAN definition"
	#elif defined(SL_LITTLEENDIAN) && SL_LITTLEENDIAN != 1
		#error "Invalid SL_LITTLEENDIAN definition"
	#elif defined(SL_BIGENDIAN) && (CXX_ARCH_LE)
		#error "Conflict at SL_BIGENDIAN and CXX_ARCH_LE definitions"
	#elif defined(SL_LITTLEENDIAN) && (CXX_ARCH_BE)
		#error "Conflict at SL_LITTLEENDIAN and CXX_ARCH_BE definitions"
	#endif
	// } @v11.4.0
	// @v11.7.3 {
	// CXX_HAVE_INTRINSIC_INT128. Определение взято у библиотеки abseil с заменой префикса ABSL-->CXX
	//
	// Checks whether the __int128 compiler extension for a 128-bit integral type is supported.
	//
	// Note: __SIZEOF_INT128__ is defined by Clang and GCC when __int128 is
	// supported, but we avoid using it in certain cases:
	// * On Clang: Building using Clang for Windows, where the Clang runtime library has 128-bit support only on LP64 architectures, but Windows is LLP64.
	// * On Nvidia's nvcc: nvcc also defines __GNUC__ and __SIZEOF_INT128__, but not all versions actually support __int128.
	#ifdef CXX_HAVE_INTRINSIC_INT128
		#error CXX_HAVE_INTRINSIC_INT128 cannot be directly set
	#elif defined(__SIZEOF_INT128__)
		#if (defined(__clang__) && !defined(_WIN32)) || (defined(__CUDACC__) && __CUDACC_VER_MAJOR__ >= 9) || (defined(__GNUC__) && !defined(__clang__) && !defined(__CUDACC__))
			#define CXX_HAVE_INTRINSIC_INT128 1
		#elif defined(__CUDACC__)
			// __CUDACC_VER__ is a full version number before CUDA 9, and is defined to a
			// string explaining that it has been removed starting with CUDA 9. We use
			// nested #ifs because there is no short-circuiting in the preprocessor.
			// NOTE: `__CUDACC__` could be undefined while `__CUDACC_VER__` is defined.
			#if __CUDACC_VER__ >= 70000
				#define CXX_HAVE_INTRINSIC_INT128 1
			#endif  // __CUDACC_VER__ >= 70000
		#endif  // defined(__CUDACC__)
	#endif
	// } @v11.7.3
	//
	// [Target OS]
	//
	#if defined(_WIN32) || defined(_WINDOWS) || defined(_WIN64) // @sobolev defined(_WIN64)
		#define CXX_OS_WINDOWS      (1)
	#else
		#define CXX_OS_WINDOWS      (0)
	#endif
	#if defined(__APPLE__)
		#include <TargetConditionals.h>
		#define CXX_OS_MAC          (TARGET_OS_MAC)
		#define CXX_OS_IOS          (TARGET_OS_IPHONE)
	#else
		#define CXX_OS_MAC          (0)
		#define CXX_OS_IOS          (0)
	#endif
	#if defined(__ANDROID__)
		#define CXX_OS_ANDROID      (1)
	#else
		#define CXX_OS_ANDROID      (0)
	#endif
	#if defined(__linux__) || defined(__ANDROID__)
		#define CXX_OS_LINUX        (1)
	#else
		#define CXX_OS_LINUX        (0)
	#endif
	#if defined(__NetBSD__)
		#define CXX_OS_NETBSD       (1)
	#else
		#define CXX_OS_NETBSD       (0)
	#endif
	#if defined(__FreeBSD__)
		#define CXX_OS_FREEBSD      (1)
	#else
		#define CXX_OS_FREEBSD      (0)
	#endif
	#if defined(__OpenBSD__)
		#define CXX_OS_OPENBSD      (1)
	#else
		#define CXX_OS_OPENBSD      (0)
	#endif
	#if defined(__DragonFly__)
		#define CXX_OS_DRAGONFLYBSD (1)
	#else
		#define CXX_OS_DRAGONFLYBSD (0)
	#endif
	#if defined(__QNXNTO__)
		#define CXX_OS_QNX          (1)
	#else
		#define CXX_OS_QNX          (0)
	#endif
	#if defined(__sun)
		#define CXX_OS_SOLARIS      (1)
	#else
		#define CXX_OS_SOLARIS      (0)
	#endif
	#if defined(__CYGWIN__)
		#define CXX_OS_CYGWIN       (1)
	#else
		#define CXX_OS_CYGWIN       (0)
	#endif
	#define CXX_OS_BSD           (CXX_OS_FREEBSD || CXX_OS_DRAGONFLYBSD || CXX_OS_NETBSD || CXX_OS_OPENBSD || CXX_OS_MAC)
	//
	// [Export|Import]
	//
	#if CXX_OS_WINDOWS
		#if(CXX_GNU || CXX_CLANG) && !CXX_MINGW
			#define CXX_EXPORT __attribute__((__dllexport__))
			#define CXX_IMPORT __attribute__((__dllimport__))
		#else
			#define CXX_EXPORT __declspec(dllexport)
			#define CXX_IMPORT __declspec(dllimport)
		#endif
	#else
		#if CXX_CLANG || CXX_GNU >= CXX_MAKE_VER(4, 0, 0) || CXX_GNU_COMPAT
			#define CXX_EXPORT __attribute__((__visibility__("default")))
			#define CXX_IMPORT
		#else
			#define CXX_EXPORT
			#define CXX_IMPORT
		#endif
	#endif
	//
	// [Function Attributes]
	// Замечания по соглашениям вызова функций:
	// STDCALL  - не применять для member-функций: снижает производительность
	// FASTCALL - Следует использовать для часто вызываемых функций с количеством параметров
	// от 1 до 3. Не совсем понятны побочные эффекты от использования регистров под параметры с точки
	// зрения оптимизации распределения регистров в вызывающих функциях.
	// @v11.2.0 Небольшое изучение замены __fastcall на __stdcall показало: 
	//  размер кода при такой замене снижается значительно, но общее быстродействие становиться на несколько процентов меньше.
	//  Пока оставляем как было.
	//  При этом есть резон standalone-функции с любым числом аргументом объявлять как __stdcall - это снизит общий размер кода.
	// CDECL - применять для функций с переменным числом аргументов для того, чтобы по-ошибке не поставили какой-либо иной вариант,
	//   то есть, это - явная декларация для тог, чтобы избежать ошибки.
	//
	#if CXX_ARCH_X86
		#if CXX_HAS_X_ATTRIBUTE
			#define CDECL      __attribute__((__cdecl__))
			#define STDCALL    __attribute__((__stdcall__))
			#define FASTCALL   __attribute__((__fastcall__))
			#define CXX_REGPARM(N) __attribute__((__regparm__(N)))
		#else
			#define CDECL      __cdecl
			#define STDCALL    __stdcall
			#define FASTCALL   __fastcall
			#define CXX_REGPARM(N)
		#endif
	#else
		#define CDECL
		#define STDCALL
		#define FASTCALL
		#define CXX_REGPARM(N)
	#endif
	#if CXX_HAS_X_ATTRIBUTE_ALWAYS_INLINE
		#define CXX_FORCEINLINE inline __attribute__((__always_inline__))
	#elif CXX_HAS_X_FORCEINLINE
		// [Comment is borrowed from abseil library]
		// We can achieve something similar to attribute((always_inline)) with MSVC by
		// using the __forceinline keyword, however this is not perfect. MSVC is
		// much less aggressive about inlining, and even with the __forceinline keyword.
		#define CXX_FORCEINLINE __forceinline
	#else
		#define CXX_FORCEINLINE inline
	#endif
	#define FORCEINLINE CXX_FORCEINLINE // @sobolev
	#if CXX_HAS_X_ATTRIBUTE_NOINLINE
		#define CXX_NOINLINE __attribute__((__noinline__))
	#elif CXX_HAS_X_DECLSPEC_NOINLINE
		#define CXX_NOINLINE __declspec(noinline)
	#else
		#define CXX_NOINLINE
	#endif
	#if CXX_HAS_X_ATTRIBUTE_NORETURN
		#define CXX_NORETURN __attribute__((__noreturn__))
	#elif CXX_HAS_X_DECLSPEC_NORETURN
		#define CXX_NORETURN __declspec(noreturn)
	#else
		#define CXX_NORETURN
	#endif
	#if CXX_HAS_X_ATTRIBUTE_UNUSEDRESULT
		#define CXX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
	#else
		#define CXX_WARN_UNUSED_RESULT 
	#endif
	#if CXX_HAS_X_ATTRIBUTE_UNUSEDPARAM
		#define CXX_UNUSED_PARAM __attribute__((__unused__))
	#elif defined(_MSC_VER) // https://github.com/harfbuzz/harfbuzz/issues/635 
		#define CXX_UNUSED_PARAM __pragma(warning(suppress: 4100 4101))
	#else
		#define CXX_UNUSED_PARAM 
	#endif
	// @v11.9.2 {
	// CXX_FUNC_PURE: The function is only allowed to read from its arguments
	//         and global memory (i.e. following a pointer argument or
	//         accessing a shared variable). The return value should
	//         only depend on its arguments, and for an identical set of
	//         arguments should return the same value.
	// 
	// CXX_FUNC_CONST: The function is only allowed to read from its arguments.
	//         It is not allowed to access global memory. The return
	//         value should only depend its arguments, and for an
	//         identical set of arguments should return the same value.
	//         This is currently the most strict function attribute.
	// 
	#if CXX_HAS_X_ATTRIBUTE_PURE
		#define CXX_FUNC_PURE  __attribute__((pure))
	#else
		#define CXX_FUNC_PURE 
	#endif
	#if CXX_HAS_X_ATTRIBUTE_CONST
		#define CXX_FUNC_CONST __attribute__((const))
	#else
		#define CXX_FUNC_CONST
	#endif
	// } @v11.9.2 
	//
	// [Likely / Unlikely]
	//
	#if CXX_HAS_X_BUILTIN_EXPECT
		#define CXX_LIKELY(EXP)   __builtin_expect(!!(EXP), 1)
		#define CXX_UNLIKELY(EXP) __builtin_expect(!!(EXP), 0)
	#else
		#define CXX_LIKELY(EXP) (EXP)
		#define CXX_UNLIKELY(EXP) (EXP)
	#endif
	#define LIKELY(EXP)   CXX_LIKELY(EXP) // @v11.0.4
	#define UNLIKELY(EXP) CXX_UNLIKELY(EXP) // @v11.0.4
	//
	// [CXX_ALIGN]
	//
	#if CXX_HAS_X_DECLSPEC_ALIGN
		#define CXX_ALIGN_TYPE(TYPE, N_ALIGN)           __declspec(align(N_ALIGN)) TYPE
		#define CXX_ALIGN_DECL(TYPE, VARIABLE, N_ALIGN) __declspec(align(N_ALIGN)) TYPE VARIABLE
	#elif CXX_HAS_X_ATTRIBUTE_ALIGNED
		#define CXX_ALIGN_TYPE(TYPE, N_ALIGN) __attribute__((__aligned__(N_ALIGN))) TYPE
		#define CXX_ALIGN_DECL(TYPE, VARIABLE, N_ALIGN) TYPE __attribute__((__aligned__(N_ALIGN))) VARIABLE
	#else
		#define CXX_ALIGN_TYPE(TYPE, N_ALIGN) TYPE
		#define CXX_ALIGN_DECL(TYPE, VARIABLE, N_ALIGN) TYPE VARIABLE
	#endif
	//
	// [Assumptions]
	//
	#if CXX_HAS_X_ASSUME
		#define CXX_ASSUME(EXP) __assume(EXP)
	#elif CXX_HAS_X_BUILTIN_ASSUME
		#define CXX_ASSUME(EXP) __builtin_assume(EXP)
	#elif CXX_HAS_X_BUILTIN_UNREACHABLE
		#define CXX_ASSUME(EXP) do { if(!(EXP)) __builtin_unreachable(); } while(0)
	#else
		#define CXX_ASSUME(EXP) ((void)0)
	#endif
	#if CXX_HAS_X_ASSUME_ALIGNED
		#define CXX_ASSUME_ALIGNED(PTR, N_ALIGN) __assume_aligned(PTR, N_ALIGN)
	#elif CXX_HAS_X_BUILTIN_ASSUME_ALIGNED
		#define CXX_ASSUME_ALIGNED(PTR, N_ALIGN) PTR = __builtin_assume_aligned(PTR, N_ALIGN)
	#else
		#define CXX_ASSUME_ALIGNED(PTR, N_ALIGN) ((void)0)
	#endif
	//
	// [Annotations]
	//
	#define CXX_UNUSED(X) (void)(X)
	// (variant from icu) #if UPRV_HAS_CPP_ATTRIBUTE(clang::fallthrough) || (UPRV_HAS_FEATURE(cxx_attributes) && UPRV_HAS_WARNING("-Wimplicit-fallthrough"))
	#if CXX_CLANG && CXX_CPLUSPLUS >= 201103L
		#define CXX_FALLTHROUGH [[clang::fallthrough]]
	#elif CXX_GNU >= CXX_MAKE_VER(7, 0, 0)
		#define CXX_FALLTHROUGH __attribute__((__fallthrough__))
	#elif (CXX_MSC >= CXX_MAKE_VER(16, 0, 0))
		// MSVC's __fallthrough annotations are checked by /analyze (Code Analysis):
		// https://msdn.microsoft.com/en-us/library/ms235402%28VS.80%29.aspx
		#include <sal.h>
		#define CXX_FALLTHROUGH __fallthrough
	#else
		#define CXX_FALLTHROUGH (void)0 // @fallthrough
	#endif
	//
	// [Other General Purpose Macros]
	//
	//@sobolev #define CXX_ARRAY_SIZE(X) (sizeof(X) / sizeof((X)[0]))
	#define CXX_OFFSET_OF(STRUCT, MEMBER) ((size_t)((const char *)&((const (STRUCT)*) 0x1)->MEMBER) - 1)
	//
	// [Guard]
	//
//
//
// Идентификация разрядности компилируемого кода
// 
#if defined(_WIN32)
	#if defined(_WIN64)
		#define __SL_PLATFORM_BIT 64
	#else
		#define __SL_PLATFORM_BIT 32
	#endif
#elif __GNUC__
	#if __x86_64__ || __ppc64__
		#define __SL_PLATFORM_BIT 64
	#else
		#define __SL_PLATFORM_BIT 32
	#endif
#endif
#ifndef SIZEOF_OFF_T
	#if defined(__VMS) && !defined(__VAX)
		#if defined(_LARGEFILE)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__OS400__) && defined(__ILEC400__)
		#if defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__MVS__) && defined(__IBMC__)
		#if defined(_LP64) || defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#elif defined(__370__) && defined(__IBMC__)
		#if defined(_LP64) || defined(_LARGE_FILES)
			#define SIZEOF_OFF_T 8
		#endif
	#endif
	#ifndef SIZEOF_OFF_T
		#define SIZEOF_OFF_T 4
	#endif
#endif
#ifndef SIZEOF_SIZE_T
	#if __SL_PLATFORM_BIT==64
		#define SIZEOF_SIZE_T 8
	#else
		#define SIZEOF_SIZE_T 4
	#endif
#endif
#ifdef __cplusplus
	// ? || (__cplusplus >= 201103L)
	#ifdef CXX_HAS_NOEXCEPT
		#define NOEXCEPT noexcept
	#else
		#define NOEXCEPT throw()
	#endif
#else
	#define NOEXCEPT throw()
#endif
// @v11.7.3 {
// restrict is standard in C99, but not in all C++ compilers
#if defined (__STDC_VERSION_) && (__STDC_VERSION__ >= 199901L) // C99
    #define _RESTRICT restrict
#elif defined(_MSC_VER) && (_MSC_VER >= 1600) // MSVC 10 or newer // there is a variant of (>=1400) in libpng
    #define _RESTRICT __restrict
#elif defined(__INTEL_COMPILER)
	#define _RESTRICT __restrict
#elif defined(__clang__) // || defined(__GNUC__)
	#define _RESTRICT __restrict__
#elif defined(__GNUC__)
	#if (__GNUC__ >= 3) // GCC 3 or newer
		#define _RESTRICT __restrict
	#else
		#define _RESTRICT __restrict__
	#endif
#elif defined(__WATCOMC__)
	#define _RESTRICT __restrict
#else // Unknown or ancient
    #define _RESTRICT
#endif
// } @v11.7.3
#if defined(_WIN32) || defined(_WIN64)
	//typedef long pid_t;
	// @v10.8.5 #define getpid GetCurrentProcessId
	// @v10.8.5 #define chdir  SetCurrentDirectory
	#ifndef STDIN_FILENO
		#define STDIN_FILENO (_fileno(stdin))
	#endif
	#ifndef STDOUT_FILENO
		#define STDOUT_FILENO (_fileno(stdout))
	#endif
	#ifndef STDERR_FILENO
		#define STDERR_FILENO (_fileno(stderr))
	#endif
	#ifdef _WIN64
		#define fseeko      _fseeki64
		#define ftello      _ftelli64
	#else
		#define fseeko      fseek
		#define ftello      ftell
	#endif
	// @variant: #define fseeko(s, o, w)	(fseek((s), static_cast<long>(o), (w)))
	// @variant: #define ftello(s)	(static_cast<long>(ftell((s))))
	#define strcasecmp  _stricmp
	#define strncasecmp _strnicmp
	#define ftruncate   _chsize_s // @v11.0.1
#endif
#if CXX_HAS_CONSTEXPR
	#define CONSTEXPR constexpr
#else
	#define CONSTEXPR
#endif
//
// @v11.2.3 {
// Аргументы функции _access() /access()/, используемые в unix-like systems
// F_OK, R_OK, W_OK точно соответствуют по значениям аналогу в Windows
//
#ifndef F_OK
	#define	F_OK	0
#endif
#ifndef R_OK
	#define	R_OK	4
#endif
#ifndef W_OK
	#define	W_OK	2
#endif
#ifndef X_OK
	#define	X_OK	1
#endif
// } @v11.2.3
// @v11.7.9 {
	#define _FILE_OFFSET_BITS 64

	#define HAVE_CTYPE_H  1
	#define HAVE_STDARG_H 1
	#define HAVE_MALLOC_H 1 // Define to 1 if you have the <malloc.h> header file
	#define HAVE_ERRNO_H  1
	#define HAVE_ZLIB_H  1 // @v11.3.12 (HAVE_ZLIB_H)-->(HAVE_ZLIB_H 1) for compatibility with libarchive
	#define HAVE_SYS_STAT_H 1
	#define HAVE__STAT
	#define HAVE_STAT     1 // @v11.8.4 (HAVE_STAT)-->(HAVE_STAT 1) for compatibility with BDB
	#define HAVE_STDLIB_H 1
	#define HAVE_STDDEF_H 1
	#define HAVE_TIME_H   1
	#define HAVE_FCNTL_H  1 // Define to 1 if you have the <fcntl.h> header file
	#define HAVE_ASSERT_H 1 /* Define to 1 if you have the <assert.h> header file. */
	/* #undef HAVE_DLFCN_H */ /* Define to 1 if you have the <dlfcn.h> header file. */
	#define HAVE_FLOOR 1 /* Define to 1 if you have the `floor' function. */
	/* #undef HAVE_GETOPT */ /* Define to 1 if you have the `getopt' function. */
	/* #undef HAVE_GLUT_GLUT_H */ /* Define to 1 if you have the <GLUT/glut.h> header file. */
	/* #undef HAVE_GL_GLUT_H */ /* Define to 1 if you have the <GL/glut.h> header file. */
	/* #undef HAVE_GL_GLU_H */ /* Define to 1 if you have the <GL/glu.h> header file. */
	/* #undef HAVE_GL_GL_H */ /* Define to 1 if you have the <GL/gl.h> header file. */
	/* #undef HAVE_INTTYPES_H */ /* Define to 1 if you have the <inttypes.h> header file. */
	#define HAVE_IO_H 1 /* Define to 1 if you have the <io.h> header file. */
	/* #undef HAVE_ISASCII */ /* Define to 1 if you have the `isascii' function. */
	/* #undef HAVE_JBG_NEWLEN */ /* Define to 1 if you have the `jbg_newlen' function. */
	#define HAVE_LFIND 1 /* Define to 1 if you have the `lfind' function. */
	#define HAVE_LIMITS_H 1 /* Define to 1 if you have the <limits.h> header file. */
	#define HAVE_MEMMOVE 1 /* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMORY_H 1 /* Define to 1 if you have the <memory.h> header file. */
	#define HAVE_MEMSET 1 /* Define to 1 if you have the `memset' function. */
	/* #undef HAVE_MMAP */ /* Define to 1 if you have the `mmap' function. */
	/* #undef HAVE_OPENGL_GLU_H */ /* Define to 1 if you have the <OpenGL/glu.h> header file. */
	/* #undef HAVE_OPENGL_GL_H */ /* Define to 1 if you have the <OpenGL/gl.h> header file. */
	#define HAVE_POW 1 /* Define to 1 if you have the `pow' function. */
	#define HAVE_SEARCH_H 1 /* Define to 1 if you have the <search.h> header file. */
	#define HAVE_SETMODE 1 /* Define to 1 if you have the `setmode' function. */
	#define HAVE_SQRT 1 /* Define to 1 if you have the `sqrt' function. */
	/* #undef HAVE_STDINT_H */ /* Define to 1 if you have the <stdint.h> header file. */
	/* #undef HAVE_STRCASECMP */ /* Define to 1 if you have the `strcasecmp' function. */
	#define HAVE_STRCHR 1 /* Define to 1 if you have the `strchr' function. */
	/* #undef HAVE_STRINGS_H */ /* Define to 1 if you have the <strings.h> header file. */
	#define HAVE_STRING_H 1 /* Define to 1 if you have the <string.h> header file. */
	#define HAVE_STRRCHR 1 // Define to 1 if you have the `strrchr' function
	#define HAVE_STRSTR 1 /* Define to 1 if you have the `strstr' function. */
	#define HAVE_STRTOL 1 // Define to 1 if you have the `strtol' function
	#define HAVE_STRTOUL 1 // Define to 1 if you have the `strtoul' function
	/* #undef HAVE_STRTOULL */ /* Define to 1 if you have the `strtoull' function. */
	/* #undef HAVE_SYS_TIME_H */ /* Define to 1 if you have the <sys/time.h> header file. */
	#define HAVE_SYS_TYPES_H 1 /* Define to 1 if you have the <sys/types.h> header file. */
	/* #undef HAVE_UNISTD_H */ /* Define to 1 if you have the <unistd.h> header file. */
	#define HAVE_LIBJPEG
	//
	/* #undef HAVE_ACL_CREATE_ENTRY */ /* Define to 1 if you have the `acl_create_entry' function. */
	/* #undef HAVE_ACL_GET_FD_NP */ /* Define to 1 if you have the `acl_get_fd_np' function. */
	/* #undef HAVE_ACL_GET_LINK */ /* Define to 1 if you have the `acl_get_link' function. */
	/* #undef HAVE_ACL_GET_LINK_NP */ /* Define to 1 if you have the `acl_get_link_np' function. */
	/* #undef HAVE_ACL_GET_PERM */ /* Define to 1 if you have the `acl_get_perm' function. */
	/* #undef HAVE_ACL_GET_PERM_NP */ /* Define to 1 if you have the `acl_get_perm_np' function. */
	/* #undef HAVE_ACL_INIT */ /* Define to 1 if you have the `acl_init' function. */
	/* #undef HAVE_ACL_LIBACL_H */ /* Define to 1 if you have the <acl/libacl.h> header file. */
	/* #undef HAVE_ACL_PERMSET_T */ /* Define to 1 if the system has the type `acl_permset_t'. */
	/* #undef HAVE_ACL_SET_FD */ /* Define to 1 if you have the `acl_set_fd' function. */
	/* #undef HAVE_ACL_SET_FD_NP */ /* Define to 1 if you have the `acl_set_fd_np' function. */
	/* #undef HAVE_ACL_SET_FILE */ /* Define to 1 if you have the `acl_set_file' function. */
	/* #undef HAVE_ARC4RANDOM_BUF */ /* Define to 1 if you have the `arc4random_buf' function. */
	/* #undef HAVE_ATTR_XATTR_H */ /* Define to 1 if you have the <attr/xattr.h> header file. */
	/* #undef HAVE_BCRYPT_H */ // Define to 1 if you have the <Bcrypt.h> header file
	#define HAVE_BCRYPT_H 1 // @v11.9.4 // Define to 1 if you have the <Bcrypt.h> header file
	/* #undef HAVE_BSDXML_H */ /* Define to 1 if you have the <bsdxml.h> header file. */
	#define HAVE_BZLIB_H 1 /* #undef HAVE_BZLIB_H */ /* Define to 1 if you have the <bzlib.h> header file. */
	/* #undef HAVE_CHFLAGS */ /* Define to 1 if you have the `chflags' function. */
	/* #undef HAVE_CHOWN */ /* Define to 1 if you have the `chown' function. */
	/* #undef HAVE_CHROOT */ /* Define to 1 if you have the `chroot' function. */
	/* #undef HAVE_COPYFILE_H */ /* Define to 1 if you have the <copyfile.h> header file. */
	/* #undef HAVE_CTIME_R */ /* Define to 1 if you have the `ctime_r' function. */
	/* #undef HAVE_CYGWIN_CONV_PATH */ /* Define to 1 if you have the `cygwin_conv_path' function. */
	/* #undef HAVE_DECL_ACE_GETACL */ /* Define to 1 if you have the declaration of `ACE_GETACL', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACE_GETACLCNT */ /* Define to 1 if you have the declaration of `ACE_GETACLCNT', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACE_SETACL */ /* Define to 1 if you have the declaration of `ACE_SETACL', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACL_SYNCHRONIZE */ /* Define to 1 if you have the declaration of `ACL_SYNCHRONIZE', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACL_TYPE_EXTENDED */ /* Define to 1 if you have the declaration of `ACL_TYPE_EXTENDED', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACL_TYPE_NFS4 */ /* Define to 1 if you have the declaration of `ACL_TYPE_NFS4', and to 0 if you don't. */
	/* #undef HAVE_DECL_ACL_USER */ /* Define to 1 if you have the declaration of `ACL_USER', and to 0 if you don't. */
	/* #undef HAVE_DECL_INT32_MAX */ /* Define to 1 if you have the declaration of `INT32_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_INT32_MIN */ /* Define to 1 if you have the declaration of `INT32_MIN', and to 0 if you don't. */
	/* #undef HAVE_DECL_INT64_MAX */ /* Define to 1 if you have the declaration of `INT64_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_INT64_MIN */ /* Define to 1 if you have the declaration of `INT64_MIN', and to 0 if you don't. */
	/* #undef HAVE_DECL_INTMAX_MAX */ /* Define to 1 if you have the declaration of `INTMAX_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_INTMAX_MIN */ /* Define to 1 if you have the declaration of `INTMAX_MIN', and to 0 if you don't. */
	/* #undef HAVE_DECL_SETACL */ /* Define to 1 if you have the declaration of `SETACL', and to 0 if you don't. */
	/* #undef HAVE_DECL_SIZE_MAX */ /* Define to 1 if you have the declaration of `SIZE_MAX', and to 0 if you don't. */
	/* Define to 1 if you have the declaration of `SSIZE_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_SSIZE_MAX */
	/* Define to 1 if you have the declaration of `strerror_r', and to 0 if you don't. */
	/* #undef HAVE_DECL_STRERROR_R */
	/* Define to 1 if you have the declaration of `UINT32_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_UINT32_MAX */
	/* Define to 1 if you have the declaration of `UINT64_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_UINT64_MAX */
	/* Define to 1 if you have the declaration of `UINTMAX_MAX', and to 0 if you don't. */
	/* #undef HAVE_DECL_UINTMAX_MAX */
	/* #undef HAVE_DECL_XATTR_NOFOLLOW */ /* Define to 1 if you have the declaration of `XATTR_NOFOLLOW', and to 0 if you don't. */
	#define HAVE_DIRECT_H 1 /* Define to 1 if you have the <direct.h> header file. */
	/* #undef HAVE_DIRENT_H */ /* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'. */
	/* #undef HAVE_DIRFD */ /* Define to 1 if you have the `dirfd' function. */
	/* #undef HAVE_DLFCN_H */ /* Define to 1 if you have the <dlfcn.h> header file. */
	/* #undef HAVE_DOPRNT */ /* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
	/* #undef HAVE_D_MD_ORDER */ /* Define to 1 if nl_langinfo supports D_MD_ORDER */
	/* #undef HAVE_EFTYPE */ /* A possible errno value for invalid file format errors */
	#define HAVE_EILSEQ 1 /* A possible errno value for invalid file format errors */
	/* #undef HAVE_EXPAT_H */ /* Define to 1 if you have the <expat.h> header file. */
	/* #undef HAVE_EXT2FS_EXT2_FS_H */ /* Define to 1 if you have the <ext2fs/ext2_fs.h> header file. */
	/* #undef HAVE_EXTATTR_GET_FILE */ /* Define to 1 if you have the `extattr_get_file' function. */
	/* #undef HAVE_EXTATTR_LIST_FILE */ /* Define to 1 if you have the `extattr_list_file' function. */
	/* #undef HAVE_EXTATTR_SET_FD */ /* Define to 1 if you have the `extattr_set_fd' function. */
	/* #undef HAVE_EXTATTR_SET_FILE */ /* Define to 1 if you have the `extattr_set_file' function. */
	/* #undef HAVE_DECL_EXTATTR_NAMESPACE_USER */ /* Define to 1 if EXTATTR_NAMESPACE_USER is defined in sys/extattr.h. */
	/* #undef HAVE_DECL_GETACL */ /* Define to 1 if you have the declaration of `GETACL', and to 0 if you don't. */
	/* #undef HAVE_DECL_GETACLCNT */ /* Define to 1 if you have the declaration of `GETACLCNT', and to 0 if you don't. */
	/* #undef HAVE_FCHDIR */ /* Define to 1 if you have the `fchdir' function. */
	/* #undef HAVE_FCHFLAGS */ /* Define to 1 if you have the `fchflags' function. */
	/* #undef HAVE_FCHMOD */ /* Define to 1 if you have the `fchmod' function. */
	/* #undef HAVE_FCHOWN */ /* Define to 1 if you have the `fchown' function. */
	/* #undef HAVE_FCNTL */ /* Define to 1 if you have the `fcntl' function. */
	/* #undef HAVE_FDOPENDIR */ /* Define to 1 if you have the `fdopendir' function. */
	/* #undef HAVE_FGETEA */ /* Define to 1 if you have the `fgetea' function. */
	/* #undef HAVE_FGETXATTR */ /* Define to 1 if you have the `fgetxattr' function. */
	/* #undef HAVE_FLISTEA */ /* Define to 1 if you have the `flistea' function. */
	/* #undef HAVE_FLISTXATTR */ /* Define to 1 if you have the `flistxattr' function. */
	/* #undef HAVE_FORK */ /* Define to 1 if you have the `fork' function. */
	/* #undef HAVE_FSEEKO */ /* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
	/* #undef HAVE_FSETEA */ /* Define to 1 if you have the `fsetea' function. */
	/* #undef HAVE_FSETXATTR */ /* Define to 1 if you have the `fsetxattr' function. */
	#define HAVE_FSTAT 1 /* Define to 1 if you have the `fstat' function. */
	/* #undef HAVE_FSTATAT */ /* Define to 1 if you have the `fstatat' function. */
	/* #undef HAVE_FSTATFS */ /* Define to 1 if you have the `fstatfs' function. */
	/* #undef HAVE_FSTATVFS */ /* Define to 1 if you have the `fstatvfs' function. */
	/* #undef HAVE_FTRUNCATE */ /* Define to 1 if you have the `ftruncate' function. */
	/* #undef HAVE_FUTIMENS */ /* Define to 1 if you have the `futimens' function. */
	/* #undef HAVE_FUTIMES */ /* Define to 1 if you have the `futimes' function. */
	/* #undef HAVE_FUTIMESAT */ /* Define to 1 if you have the `futimesat' function. */
	/* #undef HAVE_GETEA */ /* Define to 1 if you have the `getea' function. */
	/* #undef HAVE_GETEUID */ /* Define to 1 if you have the `geteuid' function. */
	/* #undef HAVE_GETGRGID_R */ /* Define to 1 if you have the `getgrgid_r' function. */
	/* #undef HAVE_GETGRNAM_R */ /* Define to 1 if you have the `getgrnam_r' function. */
	#define HAVE_GETPID 1 /* Define to 1 if you have the `getpid' function. */
	/* #undef HAVE_GETPWNAM_R */ /* Define to 1 if you have the `getpwnam_r' function. */
	/* #undef HAVE_GETPWUID_R */ /* Define to 1 if you have the `getpwuid_r' function. */
	/* #undef HAVE_GETVFSBYNAME */ /* Define to 1 if you have the `getvfsbyname' function. */
	/* #undef HAVE_GETXATTR */ /* Define to 1 if you have the `getxattr' function. */
	/* #undef HAVE_GMTIME_R */ /* Define to 1 if you have the `gmtime_r' function. */
	/* #undef HAVE_GRP_H */ /* Define to 1 if you have the <grp.h> header file. */
	/* #undef HAVE_ICONV */ /* Define to 1 if you have the `iconv' function. */
	#define HAVE_ICONV_H 0 /* Define to 1 if you have the <iconv.h> header file. */
	/* #undef HAVE_INTTYPES_H */ /* Define to 1 if you have the <inttypes.h> header file. */
	/* #undef HAVE_LANGINFO_H */ /* Define to 1 if you have the <langinfo.h> header file. */
	/* #undef HAVE_LCHFLAGS */ /* Define to 1 if you have the `lchflags' function. */
	/* #undef HAVE_LCHMOD */ /* Define to 1 if you have the `lchmod' function. */
	/* #undef HAVE_LCHOWN */ /* Define to 1 if you have the `lchown' function. */
	/* #undef HAVE_LGETEA */ /* Define to 1 if you have the `lgetea' function. */
	/* #undef HAVE_LGETXATTR */ /* Define to 1 if you have the `lgetxattr' function. */
	/* #undef HAVE_LIBACL */ /* Define to 1 if you have the `acl' library (-lacl). */
	/* #undef HAVE_LIBATTR */ /* Define to 1 if you have the `attr' library (-lattr). */
	/* #undef HAVE_LIBBSDXML */ /* Define to 1 if you have the `bsdxml' library (-lbsdxml). */
	/* #undef HAVE_LIBBZ2 */ /* Define to 1 if you have the `bz2' library (-lbz2). */
	/* #undef HAVE_LIBB2 */ /* Define to 1 if you have the `b2' library (-lb2). */
	/* #undef HAVE_BLAKE2_H */ /* Define to 1 if you have the <blake2.h> header file. */
	/* #undef HAVE_LIBCHARSET */ /* Define to 1 if you have the `charset' library (-lcharset). */
	/* #undef HAVE_LIBCRYPTO */ /* Define to 1 if you have the `crypto' library (-lcrypto). */
	/* #undef HAVE_LIBEXPAT */ /* Define to 1 if you have the `expat' library (-lexpat). */
	/* #undef HAVE_LIBGCC */ /* Define to 1 if you have the `gcc' library (-lgcc). */
	#define HAVE_LIBLZ4 /* #undef HAVE_LIBLZ4 */ /* Define to 1 if you have the `lz4' library (-llz4). */
	#define HAVE_LIBLZMA  1 /* Define to 1 if you have the `lzma' library (-llzma). */ // @v11.3.10
	/* #undef HAVE_LIBLZMADEC */ /* Define to 1 if you have the `lzmadec' library (-llzmadec). */
	/* #undef HAVE_LIBLZO2 */ /* Define to 1 if you have the `lzo2' library (-llzo2). */
	/* #undef HAVE_LIBNETTLE */ /* Define to 1 if you have the `nettle' library (-lnettle). */
	/* #undef HAVE_LIBPCRE */ /* Define to 1 if you have the `pcre' library (-lpcre). */
	/* #undef HAVE_LIBPCREPOSIX */ /* Define to 1 if you have the `pcreposix' library (-lpcreposix). */
	#define HAVE_LIBXML2            1 /* #undef HAVE_LIBXML2 */ /* Define to 1 if you have the `xml2' library (-lxml2). */
	#define HAVE_LIBXML_XMLREADER_H 1 /* #undef HAVE_LIBXML_XMLREADER_H */ /* Define to 1 if you have the <libxml/xmlreader.h> header file. */
	#define HAVE_LIBXML_XMLWRITER_H 1 /* #undef HAVE_LIBXML_XMLWRITER_H */ /* Define to 1 if you have the <libxml/xmlwriter.h> header file. */
	/* #undef HAVE_LIBZ */ /* Define to 1 if you have the `z' library (-lz). */
	#define HAVE_LIBZSTD  1 /* Define to 1 if you have the `zstd' library (-lzstd). */ // @v11.3.10
	/* #undef HAVE_LINK */ /* Define to 1 if you have the `link' function. */
	/* #undef HAVE_LINUX_FIEMAP_H */ /* Define to 1 if you have the <linux/fiemap.h> header file. */
	/* #undef HAVE_LINUX_FS_H */ /* Define to 1 if you have the <linux/fs.h> header file. */
	/* #undef HAVE_LINUX_MAGIC_H */ /* Define to 1 if you have the <linux/magic.h> header file. */
	/* #undef HAVE_LINUX_TYPES_H */ /* Define to 1 if you have the <linux/types.h> header file. */
	/* #undef HAVE_LISTEA */ /* Define to 1 if you have the `listea' function. */
	/* #undef HAVE_LISTXATTR */ /* Define to 1 if you have the `listxattr' function. */
	/* #undef HAVE_LLISTEA */ /* Define to 1 if you have the `llistea' function. */
	/* #undef HAVE_LLISTXATTR */ /* Define to 1 if you have the `llistxattr' function. */
	/* #undef HAVE_LOCALCHARSET_H */ /* Define to 1 if you have the <localcharset.h> header file. */
	/* #undef HAVE_LOCALE_CHARSET */ /* Define to 1 if you have the `locale_charset' function. */
	#define HAVE_LOCALE_H 1 /* Define to 1 if you have the <locale.h> header file. */
	/* #undef HAVE_LOCALTIME_R */ /* Define to 1 if you have the `localtime_r' function. */
	/* #undef HAVE_LONG_LONG_INT */ /* Define to 1 if the system has the type `long long int'. */
	/* #undef HAVE_LSETEA */ /* Define to 1 if you have the `lsetea' function. */
	/* #undef HAVE_LSETXATTR */ /* Define to 1 if you have the `lsetxattr' function. */
	/* #undef HAVE_LSTAT */ /* Define to 1 if you have the `lstat' function. */
	/* #undef HAVE_LSTAT_EMPTY_STRING_BUG */ /* Define to 1 if `lstat' has the bug that it succeeds when given the zero-length file name argument. */
	/* #undef HAVE_LUTIMES */ /* Define to 1 if you have the `lutimes' function. */
	/* #undef HAVE_LZ4HC_H */ 
	/* #undef HAVE_LZ4_H */ 
	#define HAVE_LZ4HC_H 1 // Define to 1 if you have the <lz4hc.h> header file
	#define HAVE_LZ4_H   1 // Define to 1 if you have the <lz4.h> header file
	/* #undef HAVE_LZMADEC_H */ /* Define to 1 if you have the <lzmadec.h> header file. */
	#define HAVE_LZMA_H 1 /* Define to 1 if you have the <lzma.h> header file. */
	/* #undef HAVE_LZMA_STREAM_ENCODER_MT */ /* Define to 1 if you have a working `lzma_stream_encoder_mt' function. */
	/* #undef HAVE_LZO_LZO1X_H */ /* Define to 1 if you have the <lzo/lzo1x.h> header file. */
	/* #undef HAVE_LZO_LZOCONF_H */ /* Define to 1 if you have the <lzo/lzoconf.h> header file. */
	/* #undef HAVE_MBRTOWC */ /* Define to 1 if you have the `mbrtowc' function. */
	/* #undef HAVE_MEMBERSHIP_H */ /* Define to 1 if you have the <membership.h> header file. */
	#define HAVE_MEMMOVE 1 /* Define to 1 if you have the `memmove' function. */
	#define HAVE_MEMORY_H 1 /* Define to 1 if you have the <memory.h> header file. */
	#define HAVE_MKDIR 1 /* Define to 1 if you have the `mkdir' function. */
	/* #undef HAVE_MKFIFO */ /* Define to 1 if you have the `mkfifo' function. */
	/* #undef HAVE_MKNOD */ /* Define to 1 if you have the `mknod' function. */
	/* #undef HAVE_MKSTEMP */ /* Define to 1 if you have the `mkstemp' function. */
	/* #undef HAVE_NDIR_H */ /* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
	/* #undef HAVE_NETTLE_AES_H */ /* Define to 1 if you have the <nettle/aes.h> header file. */
	/* #undef HAVE_NETTLE_HMAC_H */ /* Define to 1 if you have the <nettle/hmac.h> header file. */
	/* #undef HAVE_NETTLE_MD5_H */ /* Define to 1 if you have the <nettle/md5.h> header file. */
	/* #undef HAVE_NETTLE_PBKDF2_H */ /* Define to 1 if you have the <nettle/pbkdf2.h> header file. */
	/* #undef HAVE_NETTLE_RIPEMD160_H */ /* Define to 1 if you have the <nettle/ripemd160.h> header file. */
	/* #undef HAVE_NETTLE_SHA_H */ /* Define to 1 if you have the <nettle/sha.h> header file. */
	/* #undef HAVE_NL_LANGINFO */ /* Define to 1 if you have the `nl_langinfo' function. */
	/* #undef HAVE_OPENAT */ /* Define to 1 if you have the `openat' function. */
	/* #undef HAVE_PATHS_H */ /* Define to 1 if you have the <paths.h> header file. */
	/* #undef HAVE_PCREPOSIX_H */ /* Define to 1 if you have the <pcreposix.h> header file. */
	/* #undef HAVE_PIPE */ /* Define to 1 if you have the `pipe' function. */
	/* #undef HAVE_PKCS5_PBKDF2_HMAC_SHA1 */ /* Define to 1 if you have the `PKCS5_PBKDF2_HMAC_SHA1' function. */
	/* #undef HAVE_POLL */ /* Define to 1 if you have the `poll' function. */
	/* #undef HAVE_POLL_H */ /* Define to 1 if you have the <poll.h> header file. */
	/* #undef HAVE_POSIX_SPAWNP */ /* Define to 1 if you have the `posix_spawnp' function. */
	#define HAVE_PROCESS_H 1 /* Define to 1 if you have the <process.h> header file. */
	#define HAVE_PTHREAD_H 1 // Define to 1 if you have the <pthread.h> header file // @sobolev we use pthreads4w
	/* #undef HAVE_PWD_H */ /* Define to 1 if you have the <pwd.h> header file. */
	/* #undef HAVE_READDIR_R */ /* Define to 1 if you have the `readdir_r' function. */
	/* #undef HAVE_READLINK */ /* Define to 1 if you have the `readlink' function. */
	/* #undef HAVE_READLINKAT */ /* Define to 1 if you have the `readlinkat' function. */
	/* #undef HAVE_READPASSPHRASE */ /* Define to 1 if you have the `readpassphrase' function. */
	/* #undef HAVE_READPASSPHRASE_H */ /* Define to 1 if you have the <readpassphrase.h> header file. */
	/* #undef HAVE_REGEX_H */ /* Define to 1 if you have the <regex.h> header file. */
	/* #undef HAVE_SELECT */ /* Define to 1 if you have the `select' function. */
	/* #undef HAVE_SETENV */ /* Define to 1 if you have the `setenv' function. */
	#define HAVE_SETLOCALE 1 /* Define to 1 if you have the `setlocale' function. */
	/* #undef HAVE_SIGACTION */ /* Define to 1 if you have the `sigaction' function. */
	#define HAVE_SIGNAL_H  1 /* Define to 1 if you have the <signal.h> header file. */
	/* #undef HAVE_SPAWN_H */ /* Define to 1 if you have the <spawn.h> header file. */
	/* #undef HAVE_STATFS */ /* Define to 1 if you have the `statfs' function. */
	/* #undef HAVE_STATVFS */ /* Define to 1 if you have the `statvfs' function. */
	/* #undef HAVE_STAT_EMPTY_STRING_BUG */ /* Define to 1 if `stat' has the bug that it succeeds when given the zero-length file name argument. */
	/* #undef HAVE_STDINT_H */ /* Define to 1 if you have the <stdint.h> header file. */
	#define HAVE_STRDUP 1 /* Define to 1 if you have the `sstrdup' function. */
	#define HAVE_STRERROR 1 /* Define to 1 if you have the `strerror' function. */
	/* #undef HAVE_STRERROR_R */ /* Define to 1 if you have the `strerror_r' function. */
	#define HAVE_STRFTIME 1 /* Define to 1 if you have the `strftime' function. */
	/* #undef HAVE_STRINGS_H */ /* Define to 1 if you have the <strings.h> header file. */
	/* #undef HAVE_STRUCT_STATFS_F_NAMEMAX */ /* Define to 1 if `f_namemax' is a member of `struct statfs'. */
	/* #undef HAVE_STRUCT_STATVFS_F_IOSIZE */ /* Define to 1 if `f_iosize' is a member of `struct statvfs'. */
	/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIME */ /* Define to 1 if `st_birthtime' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC */ /* Define to 1 if `st_birthtimespec.tv_nsec' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_BLKSIZE */ /* Define to 1 if `st_blksize' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_FLAGS */ /* Define to 1 if `st_flags' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC */ /* Define to 1 if `st_mtimespec.tv_nsec' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_MTIME_N */ /* Define to 1 if `st_mtime_n' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_MTIME_USEC */ /* Define to 1 if `st_mtime_usec' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC */ /* Define to 1 if `st_mtim.tv_nsec' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_STAT_ST_UMTIME */ /* Define to 1 if `st_umtime' is a member of `struct stat'. */
	/* #undef HAVE_STRUCT_TM_TM_GMTOFF */ /* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
	/* #undef HAVE_STRUCT_TM___TM_GMTOFF */ /* Define to 1 if `__tm_gmtoff' is a member of `struct tm'. */
	/* #undef HAVE_STRUCT_VFSCONF */ /* Define to 1 if you have `struct vfsconf'. */
	/* #undef HAVE_STRUCT_XVFSCONF */ /* Define to 1 if you have `struct xvfsconf'. */
	/* #undef HAVE_SYMLINK */ /* Define to 1 if you have the `symlink' function. */
	/* #undef HAVE_SYS_ACL_H */ /* Define to 1 if you have the <sys/acl.h> header file. */
	/* #undef HAVE_SYS_CDEFS_H */ /* Define to 1 if you have the <sys/cdefs.h> header file. */
	/* #undef HAVE_SYS_DIR_H */ /* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'. */
	/* #undef HAVE_SYS_EA_H */ /* Define to 1 if you have the <sys/ea.h> header file. */
	/* #undef HAVE_SYS_EXTATTR_H */ /* Define to 1 if you have the <sys/extattr.h> header file. */
	/* #undef HAVE_SYS_IOCTL_H */ /* Define to 1 if you have the <sys/ioctl.h> header file. */
	/* #undef HAVE_SYS_MKDEV_H */ /* Define to 1 if you have the <sys/mkdev.h> header file. */
	/* #undef HAVE_SYS_MOUNT_H */ /* Define to 1 if you have the <sys/mount.h> header file. */
	/* #undef HAVE_SYS_NDIR_H */ /* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'. */
	/* #undef HAVE_SYS_PARAM_H */ /* Define to 1 if you have the <sys/param.h> header file. */
	/* #undef HAVE_SYS_POLL_H */ /* Define to 1 if you have the <sys/poll.h> header file. */
	/* #undef HAVE_SYS_RICHACL_H */ /* Define to 1 if you have the <sys/richacl.h> header file. */
	/* #undef HAVE_SYS_SELECT_H */ /* Define to 1 if you have the <sys/select.h> header file. */
	/* #undef HAVE_SYS_STATFS_H */ /* Define to 1 if you have the <sys/statfs.h> header file. */
	/* #undef HAVE_SYS_STATVFS_H */ /* Define to 1 if you have the <sys/statvfs.h> header file. */
	/* #undef HAVE_SYS_SYSMACROS_H */ /* Define to 1 if you have the <sys/sysmacros.h> header file. */
	/* #undef HAVE_SYS_TIME_H */ /* Define to 1 if you have the <sys/time.h> header file. */
	#define HAVE_SYS_UTIME_H 1 /* Define to 1 if you have the <sys/utime.h> header file. */
	/* #undef HAVE_SYS_UTSNAME_H */ /* Define to 1 if you have the <sys/utsname.h> header file. */
	/* #undef HAVE_SYS_VFS_H */ /* Define to 1 if you have the <sys/vfs.h> header file. */
	/* #undef HAVE_SYS_WAIT_H */ /* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
	/* #undef HAVE_SYS_XATTR_H */ /* Define to 1 if you have the <sys/xattr.h> header file. */
	/* #undef HAVE_TIMEGM */ /* Define to 1 if you have the `timegm' function. */
	#define HAVE_TZSET 1 /* Define to 1 if you have the `tzset' function. */
	/* #undef HAVE_UNISTD_H */ /* Define to 1 if you have the <unistd.h> header file. */
	/* #undef HAVE_UNSETENV */ /* Define to 1 if you have the `unsetenv' function. */
	/* #undef HAVE_UNSIGNED_LONG_LONG */ /* Define to 1 if the system has the type `unsigned long long'. */
	/* #undef HAVE_UNSIGNED_LONG_LONG_INT */ /* Define to 1 if the system has the type `unsigned long long int'. */
	#define HAVE_UTIME 1 /* Define to 1 if you have the `utime' function. */
	/* #undef HAVE_UTIMENSAT */ /* Define to 1 if you have the `utimensat' function. */
	/* #undef HAVE_UTIMES */ /* Define to 1 if you have the `utimes' function. */
	/* #undef HAVE_UTIME_H */ /* Define to 1 if you have the <utime.h> header file. */
	/* #undef HAVE_VFORK */ /* Define to 1 if you have the `vfork' function. */
	#define HAVE_SNPRINTF  1 // Define to 1 if you have the `snprintf' function
	#define HAVE_VFPRINTF  1
	#define HAVE_VPRINTF   1 // Define to 1 if you have the `vprintf' function
	#define HAVE_VASPRINTF 1
	#define HAVE_SPRINTF_L 1
	#define HAVE_WCHAR_H 1 /* Define to 1 if you have the <wchar.h> header file. */
	#define HAVE_WCHAR_T 1 /* Define to 1 if the system has the type `wchar_t'. */
	/* #undef HAVE_WCRTOMB */ /* Define to 1 if you have the `wcrtomb' function. */
	#define HAVE_WCSCMP 1 /* Define to 1 if you have the `wcscmp' function. */
	#define HAVE_WCSCPY 1 /* Define to 1 if you have the `wcscpy' function. */
	#define HAVE_WCSLEN 1 /* Define to 1 if you have the `wcslen' function. */
	#define HAVE_WCTOMB 1 // Define to 1 if you have the `wctomb' function
	#define HAVE_WCTYPE_H 1 /* Define to 1 if you have the <wctype.h> header file. */
	#define HAVE_WINCRYPT_H 1 /* Define to 1 if you have the <wincrypt.h> header file. */
	#define HAVE_WINDOWS_H 1 /* Define to 1 if you have the <windows.h> header file. */
	#define HAVE_WINIOCTL_H 1 /* Define to 1 if you have the <winioctl.h> header file. */
	#define HAVE__CrtSetReportMode 1 /* Define to 1 if you have _CrtSetReportMode in <crtdbg.h>  */
	/* #undef HAVE_WMEMCMP */ /* Define to 1 if you have the `wmemcmp' function. */
	/* #undef HAVE_WMEMCPY */ /* Define to 1 if you have the `wmemcpy' function. */
	/* #undef HAVE_WMEMMOVE */ /* Define to 1 if you have the `wmemmove' function. */
	/* #undef HAVE_WORKING_EXT2_IOC_GETFLAGS */ /* Define to 1 if you have a working EXT2_IOC_GETFLAGS */
	/* #undef HAVE_WORKING_FS_IOC_GETFLAGS */ /* Define to 1 if you have a working FS_IOC_GETFLAGS */
	#if _MSC_VER >= 1900
		#define HAVE_ZSTD_H 1 // @v11.3.11 Define to 1 if you have the <zstd.h> header file.
	#endif
	/* #undef HAVE__CTIME64_S */ /* Define to 1 if you have the `_ctime64_s' function. */
	/* #undef HAVE__FSEEKI64 */ /* Define to 1 if you have the `_fseeki64' function. */
	/* #undef HAVE__GET_TIMEZONE */ /* Define to 1 if you have the `_get_timezone' function. */
	/* #undef HAVE__LOCALTIME64_S */ /* Define to 1 if you have the `_localtime64_s' function. */
	/* #undef HAVE__MKGMTIME64 */ /* Define to 1 if you have the `_mkgmtime64' function. */
	//
	#define HAVE_MATH_H            1 
	#define HAVE_MEMCPY            1
	#define HAVE_STPCPY
	#define HAVE_STRNLEN           1
	#define HAVE_TIME_T_IN_TIME_H
	#define HAVE_FDOPEN
	#define HAVE_LUA
	#define HAVE_STDBOOL_H 1
	#define HAVE__BOOL

	#define HAVE_STRTOD
	#define HAVE_SSCANF
	#define HAVE_STRTOD_L
	#define HAVE_SYS_TIMEB_H
	#define HAVE_FTIME
	#define HAVE_GMTIME
	#define HAVE_MBTOWC
// } @v11.7.9
#endif // } __SLPORT_H
