// scintilla-internal.h 
//
#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <cstdlib>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <cmath>
#include <forward_list>
#include <chrono>
#include <climits>
#include <new>
#include <mutex>
#include <cstdarg>
#include <ctime>
#include <iterator>
#include <memory>
#include <cctype>

#include <windows.h>

#include "Platform.h"
#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "Position.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"
#include "PropSetSimple.h"
#include "LexerBase.h"
#include "LexerSimple.h"
#include "CharacterCategory.h"

#undef min
#undef max
