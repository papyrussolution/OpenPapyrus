// xlslib-internal.h
//
#include <slib.h>
#ifdef HAVE_STDINT_H
	#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#endif
#if defined(__cplusplus)					// ALL C++ users
#endif
#ifdef HAVE_STRING_H
	#include <string.h>	
#endif
#ifdef HAVE_MALLOC_H
	#include <malloc.h>	
#endif
#ifdef HAVE_MEMORY_H
	#include <memory.h>	
#endif
#if defined(__cplusplus)					// ALL C++ users
	#include <string>
	#include <cstring>    
	#include <sstream>
	#include <iostream>
	#include <fstream>
	#include <list>
	#include <vector>
	#include <set>
	#include <algorithm>
#endif

namespace xlslib_core {
	class CDataStorage;
	class CUnit;
	class hpsf_doc_t;
	class CGlobalRecords;
	class CHPSFdoc;
	class estimated_formula_result_t;
	class insertsort2;
	class range_t;
	class COleProp;
	class oleSort;
	class xf_t;
	class CSummaryInfo;
	class CDocSummaryInfo;
	class worksheet;
	class cell_t;
	class range;
	class expression_node_t;
};

#include "xlstypes.h"
#include "xlsys.h"
#include "systype.h"
#include "unit.h"	// superclass
#include "record.h"
#include "colors.h"
#include "font.h"
#include "format.h"
#include "extformat.h"
#include "cell.h"
#include "blank.h"
#include "row.h"	// has many needed defines used in this file
#include "recdef.h"
#include "datast.h"
#include "rectypes.h"
#include "binfile.h"
#include "tostr.h"
namespace xlslib_core {
	//#include "biffsection.h"
	class CBiffSection {
	public:
		CBiffSection() {}
		virtual ~CBiffSection() {}
	};
}
#include "label.h"
#include "globalrec.h"
#include "colinfo.h"
#include "colors.h"
#include "continue.h"
#include "number.h"
#include "boolean.h"
#include "err.h"
#include "note.h"
#include "merged.h"
#include "index.h"
#include "HPSF.h"
#include "summinfo.h"        // pseudo base class
#include "docsumminfo.h"
#include "formula_const.h"
#include "formula.h"
#include "formula_estimate.h"
#include "formula_cell.h"
#include "formula_expr.h"
#include "common.h"
#include "sheetrec.h"
#include "oleprop.h"
#include "olefs.h"
#include "oledoc.h"
#include "stringtok.h"
#include "range.h"
#include "workbook.h"
