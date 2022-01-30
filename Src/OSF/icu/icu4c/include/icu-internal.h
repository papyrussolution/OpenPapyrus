// ICU-INTERNAL.H
//
#ifndef __ICU_INTERNAL_H
#define __ICU_INTERNAL_H // {

#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>

#define U_I18N_IMPLEMENTATION
#define U_IO_IMPLEMENTATION

//#include <stdlib.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <string.h>
//#include <ctype.h>
//#include <float.h>
//#include <limits.h>
//#include <locale.h>
//#include <assert.h>
//#include <time.h>
/*
#include <typeinfo>
#include <array>
#include <utility>
#include <cmath>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <algorithm>
#include <limits>
#include <functional>
#include <string>
#include <cstddef>
#include <memory>
#include <set>
#include <fstream>
#include <type_traits>
#include <iterator>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <cinttypes>
*/

#include "utypeinfo.h"

#include "unicode/utypes.h"
#include "unicode/icudataver.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "unicode/utf8.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/alphaindex.h"
#include "unicode/coll.h"
#include "unicode/localpointer.h"
#include "unicode/normalizer2.h"
#include "unicode/tblcoll.h"
#include "unicode/uchar.h"
#include "unicode/ulocdata.h"
#include "unicode/uobject.h"
#include "unicode/appendable.h"
#include "unicode/usetiter.h"
#include "unicode/chariter.h"
#include "unicode/ures.h"
#include "unicode/udata.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/uscript.h"
#include "unicode/ucharstrie.h"
#include "unicode/bytestrie.h"
#include "unicode/rbbi.h"
#include "unicode/brkiter.h"
#include "unicode/filteredbrk.h"
#include "unicode/bytestream.h"
#include "unicode/edits.h"
#include "unicode/stringoptions.h"
#include "unicode/ustream.h"
#include "unicode/unum.h"
#include "unicode/udat.h"
#include "unicode/regex.h"
#include "unicode/uclean.h"
#include "unicode/unifilt.h"
#include "unicode/uversion.h"
#include "unicode/choicfmt.h"
#include "unicode/numfmt.h"
#include "unicode/locid.h"
#include "unicode/coleitr.h"
#include "unicode/caniter.h"
#include "unicode/parseerr.h"
#include "unicode/ucol.h"
#include "unicode/ustringtrie.h"
#include "unicode/ucharstriebuilder.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/ucurr.h"
#include "unicode/numsys.h"
#include "unicode/uenum.h"
#include "unicode/uloc.h"
#include "unicode/resbund.h"
#include "unicode/measunit.h"
#include "unicode/measure.h"
#include "unicode/errorcode.h"
#include "unicode/messagepattern.h"
#include "unicode/rbnf.h"
#include "unicode/selfmt.h"
#include "unicode/ucnv_err.h"
#include "unicode/umsg.h"
#include "unicode/datefmt.h"
#include "unicode/msgfmt.h"
// (produces error in number_skeletons.cpp) #include "unicode/numberformatter.h"
#include "unicode/plurrule.h"
#include "unicode/plurfmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/fieldpos.h"
#include "unicode/umisc.h"
#include "unicode/fpositer.h"
#include "unicode/scientificnumberformatter.h"
#include "unicode/idna.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/uidna.h"
#include "unicode/ucal.h"
#include "unicode/timezone.h"
// #include "unicode/ucnv.h"
#include "unicode/localematcher.h"
#include "unicode/localebuilder.h"
#include "unicode/strenum.h"
#include "unicode/sortkey.h"
#include "unicode/chariter.h"
#include "unicode/uchriter.h"
#include "unicode/schriter.h"
#include "unicode/ubrk.h"
#include "unicode/parsepos.h"
#include "unicode/calendar.h"
#include "unicode/curramt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/dtptngen.h"
#include "unicode/fmtable.h"
#include "unicode/format.h"
#include "unicode/gregocal.h"
#include "unicode/locdspnm.h"
#include "unicode/normlzr.h"
#include "unicode/simpletz.h"
#include "unicode/stsearch.h"
#include "unicode/tmunit.h"
#include "unicode/translit.h"
#include "unicode/unifunct.h"
#include "unicode/simpleformatter.h"
#include "unicode/umutablecptrie.h"
#include "unicode/utext.h"
#include "unicode/utrace.h"
// #include "unicode/numberrangeformatter.h"
#include "unicode/ufieldpositer.h"
#include "unicode/uformattedvalue.h"
#include "unicode/unumberformatter.h"
#include "unicode/dtrule.h"
#include "unicode/tzrule.h"
#include "unicode/tztrans.h"
#include "unicode/basictz.h"
#include "unicode/rbtz.h"
#include "unicode/tznames.h"
#include "unicode/ucpmap.h"
#include "unicode/ucptrie.h"
#include "unicode/uset.h"
#include "unicode/utrans.h"
#include "unicode/rep.h"
#include "unicode/udisplaycontext.h"
#include "unicode/dtintrv.h"
#include "unicode/dtitvinf.h"
#include "unicode/formattedvalue.h"
#include "unicode/dtitvfmt.h"
#include "unicode/udateintervalformat.h"
#include "unicode/ucoleitr.h"
#include "unicode/uiter.h"
#include "unicode/unorm.h"
#include "unicode/unorm2.h"
#include "unicode/gender.h"
#include "unicode/ugender.h"
#include "unicode/currunit.h"
#include "unicode/uregex.h"
#include "unicode/listformatter.h"
#include "unicode/ulistformatter.h"
#include "unicode/vtzone.h"
//#include "unicode/ustdio.h"

#include "cmemory.h"
#include "bmpset.h"
#include "uassert.h"
#include "cstring.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "brkeng.h"
#include "dictbe.h"
#include "lstmbe.h"
#include "charstr.h"
#include "dictionarydata.h"
#include "mutex.h"
#include "umutex.h"
#include "uresimp.h"
#include "ubrkimpl.h"
#include "ucln_cmn.h"
#include "servloc.h"
#include "locbased.h"
// #include "utracimp.h"
#include "bytesinkutil.h"
#include "cstr.h"
#include "uhash.h"
#include "pluralmap.h"
#include "util.h"
#include "udataswp.h"
#include "uset_imp.h"
#include "normalizer2impl.h"
#include "collation.h"
#include "putilimp.h"
#include "utrie2.h"
#include "collationdata.h"
#include "csmatch.h"
#include "uarrsort.h"
// 
// Integer division and modulo with negative numerators
// yields negative modulo results and quotients that are one more than
// what we need here.
// This macro adjust the results so that the modulo-value m is always >=0.
// For positive n, the if() condition is always FALSE.
// @param n Number to be split into quotient and rest. Will be modified to contain the quotient.
// @param d Divisor.
// @param m Output variable for the rest (modulo result).
// 
#define NEGDIVMOD(n, d, m) UPRV_BLOCK_MACRO_BEGIN { \
		(m) = (n)%(d); \
		(n) /= (d); \
		if((m) < 0) { \
			--(n); \
			(m) += (d); \
		} \
} UPRV_BLOCK_MACRO_END

#include "anytrans.h"
#include "nultrans.h"
#include "tridpars.h"
#include "uinvchar.h"
#include "bocsu.h"

#endif // } __ICU_INTERNAL_H
