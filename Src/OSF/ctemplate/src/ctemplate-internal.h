// ctemplate-internal.h
//
#ifndef __CTEMPLATE_INTERNAL_H
#define __CTEMPLATE_INTERNAL_H

#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>

#include <config.h>
#include "windows/config.h"
#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#endif // another place uintptr_t might be
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#include HASH_MAP_H
#include HASH_SET_H
#include "base/mutex.h" // This has to come first to get _XOPEN_SOURCE
#include "base/arena.h"
#include "base/arena-inl.h"
#include "base/util.h"
#include "base/arena.h"
#include "base/thread_annotations.h"
#include "base/macros.h"
#include "base/fileutil.h"
#include "base/small_map.h"
#include <ctemplate/find_ptr.h>
#include <ctemplate/template_string.h>
#include <ctemplate/template_pathops.h>
#include <ctemplate/template_namelist.h>
#include <ctemplate/template.h>
#include <ctemplate/template_modifiers.h>
#include <ctemplate/per_expand_data.h>
#include <ctemplate/template_dictionary.h>
#include <ctemplate/template_cache.h>
#include <ctemplate/template_enums.h>
#include <ctemplate/template_annotator.h>
#include <ctemplate/template_emitter.h>
#include <ctemplate/template_dictionary_interface.h>
#include "template_modifiers_internal.h"
#include "htmlparser/jsparser.h"
#include "htmlparser/statemachine.h"
#include "htmlparser/htmlparser.h"
#include "htmlparser/htmlparser_cpp.h"
#include "indented_writer.h"

#ifndef PATH_MAX
	#ifdef MAXPATHLEN
		#define PATH_MAX        MAXPATHLEN
	#else
		#define PATH_MAX        4096 // seems conservative for max filename len!
	#endif
#endif

#endif // __CTEMPLATE_INTERNAL_H