// OpenXLSX-internal.hpp
//
#ifndef OPENXLSX_INTERNAL_HPP
#define OPENXLSX_INTERNAL_HPP

#include <slib.h>

#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <cstring>
#include <array>
#include <cmath>
#ifdef CHARCONV_ENABLED
	#include <charconv>
#endif
#include <cctype>
#include <cstdint>
#include <ios>
#include <cassert>
#include <sstream>
#include <iterator>
#include <vector>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <limits>
#include <map>

#include <../osf/pugixml/pugixml.hpp>

#include "XLConstants.hpp"
#include "XLCell.hpp"
#include "XLCellValue.hpp"
#include "XLCellIterator.hpp"
#include "XLCellRange.hpp"
#include "XLCellReference.hpp"
#include "XLException.hpp"
#include "utilities/XLUtilities.hpp"
#include "XLDocument.hpp"
#include "XLDrawing.hpp"
#include "XLComments.hpp"
#include "XLDateTime.hpp"
#include "XLXmlParser.hpp"
#include "XLXmlFile.hpp"
#include "XLXmlData.hpp"
#include "XLSheet.hpp"
#include "XLWorkbook.hpp"
#include "XLColor.hpp"
#include "XLStyles.hpp"
#include "XLColumn.hpp"
#include "XLMergeCells.hpp"
#include "XLTables.hpp"
#include "XLRow.hpp"
#include "XLRowData.hpp"
#include "XLProperties.hpp"
#include "XLRelationships.hpp"
#include "XLFormula.hpp"
#include "XLContentTypes.hpp"

#endif // !OPENXLSX_INTERNAL_HPP