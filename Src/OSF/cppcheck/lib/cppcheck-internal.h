// cppcheck-internal.h 
//
#ifndef __CPPCHECK_INTERNAL_H
#define __CPPCHECK_INTERNAL_H

#define HAVE_RULES
#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#include <simplecpp.h>
#include <cppcheck-config.h>
#include "check.h"
#include "library.h"
#include "token.h"
#include "tokenize.h"
#include "tokenlist.h"
#include "valueflow.h"
#include "symboldatabase.h"
#include "settings.h"
#include "checktype.h"
#include "checkclass.h"
#include "astutils.h"
#include "standards.h"
#include "errorlogger.h"
#include "errortypes.h"
/**
 * CWE id (Common Weakness Enumeration)
 * See https://cwe.mitre.org/ for further reference.
 * */
// CWE list: https://cwe.mitre.org/data/published/cwe_v3.4.1.pdf
const struct CWE CWE119(119U);   // Improper Restriction of Operations within the Bounds of a Memory Buffer
const struct CWE CWE128(128U);   // Wrap-around Error
const struct CWE CWE131(131U);   // Incorrect Calculation of Buffer Size
//const struct CWE CWE131(131U);   // Incorrect Calculation of Buffer Size
const struct CWE CWE170(170U);   // Improper Null Termination
const struct CWE CWE190(190U);   // Integer Overflow or Wraparound
const struct CWE CWE195(195U);   // Signed to Unsigned Conversion Error
const struct CWE CWE197(197U);   // Numeric Truncation Error
//const struct CWE CWE197(197U);   // Numeric Truncation Error
const struct CWE CWE252(252U);   // Unchecked Return Value
const struct CWE CWE362(362U);   // Concurrent Execution using Shared Resource with Improper Synchronization ('Race Condition')
const struct CWE CWE369(369U);   // Divide By Zero
const struct CWE uncheckedErrorConditionCWE(391U);
const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);  // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);  // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);  // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);  // Indicator of Poor Code Quality
//const struct CWE CWE398(398U);  // Indicator of Poor Code Quality
const struct CWE CWE_ARGUMENT_SIZE(398U);  // Indicator of Poor Code Quality
const struct CWE CWE_ARRAY_INDEX_THEN_CHECK(398U);  // Indicator of Poor Code Quality
const struct CWE CWE401(401U);   // Improper Release of Memory Before Removing Last Reference ('Memory Leak')
const struct CWE CWE404(404U);   // Improper Resource Shutdown or Release
const struct CWE CWE415(415U);
const struct CWE CWE_USE_OF_UNINITIALIZED_VARIABLE(457U);
const struct CWE CWE467(467U);   // Use of sizeof() on a Pointer Type
const struct CWE CWE475(475U);   // Undefined Behavior for Input to API
const struct CWE CWE_NULL_POINTER_DEREFERENCE(476U);
const struct CWE CWE477(477U);  // Use of Obsolete Functions
const struct CWE CWE480(480U);   // Use of Incorrect Operator
const struct CWE CWE482(482U);   // Comparing instead of Assigning
const struct CWE CWE561(561U);   // Dead Code
//const struct CWE CWE561(561U);   // Dead Code
const struct CWE CWE562(562U);   // Return of Stack Variable Address
const struct CWE CWE563(563U);   // Assignment to Variable without Use ('Unused Variable')
//const struct CWE CWE563(563U);   // Assignment to Variable without Use ('Unused Variable')
const struct CWE CWE570(570U);   // Expression is Always False
//const struct CWE CWE570(570U);   // Expression is Always False
//const struct CWE CWE570(570U);   // Expression is Always False
const struct CWE CWE571(571U);   // Expression is Always True
//const struct CWE CWE571(571U);   // Expression is Always True
//const struct CWE CWE571(571U);   // Expression is Always True
//const struct CWE CWE571(571U);  // Expression is Always True
const struct CWE CWE587(587U);   // Assignment of a Fixed Address to a Pointer
const struct CWE CWE590(590U);   // Free of Memory not on the Heap
const struct CWE CWE595(595U);   // Comparison of Object References Instead of Object Contents
const struct CWE CWE597(597U);   // Use of Wrong Operator in String Comparison
const struct CWE CWE628(628U);   // Function Call with Incorrectly Specified Arguments
//const struct CWE CWE628(628U);   // Function Call with Incorrectly Specified Arguments
//const struct CWE CWE628(628U);   // Function Call with Incorrectly Specified Arguments
//const struct CWE CWE628(628U);  // Function Call with Incorrectly Specified Arguments
const struct CWE CWE664(664U);   // Improper Control of a Resource Through its Lifetime
//const struct CWE CWE664(664U);   // Improper Control of a Resource Through its Lifetime
//const struct CWE CWE664(664U);
//const struct CWE CWE664(664U);  // Improper Control of a Resource Through its Lifetime
const struct CWE CWE665(665U);   // Improper Initialization
//const struct CWE CWE665(665U);   // Improper Initialization
//const struct CWE CWE665(665U);   // Improper Initialization
const struct CWE CWE667(667U);   // Improper Locking
const struct CWE CWE672(672U);   // Operation on a Resource after Expiration or Release
//const struct CWE CWE672(672U);
const struct CWE CWE_USE_OF_POTENTIALLY_DANGEROUS_FUNCTION(676U);
const struct CWE CWE682(682U);   // Incorrect Calculation
//const struct CWE CWE682(682U);   // Incorrect Calculation
const struct CWE CWE_INCORRECT_CALCULATION(682U);
const struct CWE CWE683(683U);   // Function Call With Incorrect Order of Arguments
const struct CWE CWE685(685U);   // Function Call With Incorrect Number of Arguments
const struct CWE CWE686(686U);   // Function Call With Incorrect Argument Type
//const struct CWE CWE686(686U);   // Function Call With Incorrect Argument Type
//const struct CWE CWE686(686U);  // Function Call With Incorrect Argument Type
const struct CWE CWE687(687U);   // Function Call With Incorrectly Specified Argument Value
//const struct CWE CWE687(687U);  // Function Call With Incorrectly Specified Argument Value
const struct CWE CWE688(688U);   // Function Call With Incorrect Variable or Reference as Argument
//const struct CWE CWE688(688U);  // Function Call With Incorrect Variable or Reference as Argument
const struct CWE CWE703(703U);   // Improper Check or Handling of Exceptional Conditions
const struct CWE CWE704(704U);   // Incorrect Type Conversion or Cast
//const struct CWE CWE704(704U);   // Incorrect Type Conversion or Cast
//const struct CWE CWE704(704U);   // Incorrect Type Conversion or Cast
//const struct CWE CWE704(704U);  // Incorrect Type Conversion or Cast
const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
//const struct CWE CWE758(758U);  // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
const struct CWE CWE_POINTER_ARITHMETIC_OVERFLOW(758U); // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
const struct CWE CWE_ONE_DEFINITION_RULE(758U);
const struct CWE CWE762(762U);   // Mismatched Memory Management Routines
//const struct CWE CWE762(762U);   // Mismatched Memory Management Routines
const struct CWE CWE768(768U);   // Incorrect Short Circuit Evaluation
const struct CWE CWE771(771U);  // Missing Reference to Active Allocated Resource
const struct CWE CWE772(772U);  // Missing Release of Resource after Effective Lifetime
const struct CWE CWE783(783U);   // Operator Precedence Logic Error
const struct CWE CWE786(786U);   // Access of Memory Location Before Start of Buffer
const struct CWE CWE_BUFFER_UNDERRUN(786U);  // Access of Memory Location Before Start of Buffer
const struct CWE CWE788(788U);   // Access of Memory Location After End of Buffer
const struct CWE CWE_BUFFER_OVERRUN(788U);   // Access of Memory Location After End of Buffer
const struct CWE CWE825(825U);   // Expired Pointer Dereference
const struct CWE CWE_EXPIRED_POINTER_DEREFERENCE(825U);
const struct CWE CWE833(833U);   // Deadlock
const struct CWE CWE834(834U);   // Excessive Iteration
const struct CWE CWE910(910U);  // Use of Expired File Descriptor

#include "mathlib.h"
#include "utils.h"
#include "analyzerinfo.h"
#include "path.h"
#include "infer.h"
#include "valueptr.h"
#include "analyzer.h"
#include "calculate.h"
#include "ctu.h"
#include "checkuninitvar.h"
#include "forwardanalyzer.h"
#include "platform.h"
#include "programmemory.h"
#include "reverseanalyzer.h"
#include "sourcelocation.h"
#include "checksizeof.h"
#include "checkother.h"
#include "checkpostfixoperator.h"
#include "checkstl.h"
#include "pathanalysis.h"
#include "checknullpointer.h"
#include "preprocessor.h"
#include "checkleakautovar.h"
#include "checkassert.h"
#include "checkautovariables.h"
#include "checkbool.h"
#include "checkboost.h"
#include "checkbufferoverrun.h"
#include "checkcondition.h"
#include "checkexceptionsafety.h"
#include "checkfunctions.h"
#include "checkio.h"
#include "pathmatch.h"
#include "checkmemoryleak.h"
#include "checkstring.h"
#include "checkunusedfunctions.h"
#include "checkunusedvar.h"

#include <tinyxml2.h>

#endif // !__CPPCHECK_INTERNAL_H

