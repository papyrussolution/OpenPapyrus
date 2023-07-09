// jbig2dec-internal.h
//
#include <slib.h>
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#define HAVE_LIBPNG // @v11.6.6
//#include "os_types.h"
	#if defined(HAVE_CONFIG_H)
		#include "config_types.h"
	#elif defined(_WIN32)
		#include "config_win32.h"
	#elif defined (STD_INT_USE_SYS_TYPES_H)
		#include <sys/types.h>
	#elif defined (STD_INT_USE_INTTYPES_H)
		#include <inttypes.h>
	#elif defined (STD_INT_USE_SYS_INTTYPES_H)
		#include <sys/inttypes.h>
	#elif defined (STD_INT_USE_SYS_INT_TYPES_H)
		#include <sys/int_types.h>
	#else
		#include <stdint.h>
	#endif
//
#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_arith.h"
#include "jbig2_arith_int.h"
#include "jbig2_arith_iaid.h"
#include "jbig2_generic.h"
#include "jbig2_huffman.h"
#include "jbig2_hufftab.h"
#include "jbig2_image.h"
#include "jbig2_image_rw.h"
#include "jbig2_halftone.h"
#include "jbig2_mmr.h"
#include "jbig2_refinement.h"
#include "jbig2_page.h"
#include "jbig2_segment.h"
#include "jbig2_symbol_dict.h"
#include "jbig2_text.h"