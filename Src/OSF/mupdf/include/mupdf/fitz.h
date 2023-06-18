#ifndef MUDPF_FITZ_H
#define MUDPF_FITZ_H

#define SHARE_JPEG 1 // @sobolev @20230428 (предполагаю, что это решит проблему с авариями при работе с jpeg из других модулей)

#include <slib.h>

#ifdef __cplusplus
// @sobolev extern "C" {
#endif

#include "mupdf/fitz/version.h"
#include "mupdf/fitz/config.h"
#include "mupdf/fitz/system.h"
#include "mupdf/fitz/geometry.h"
#include "mupdf/fitz/context.h"
#include "mupdf/fitz/buffer.h"
#include "mupdf/fitz/string-util.h"
#include "mupdf/fitz/stream.h"
#include "mupdf/fitz/output.h"
#include "mupdf/fitz/log.h"
#include "mupdf/fitz/crypt.h"
#include "mupdf/fitz/getopt.h"
#include "mupdf/fitz/hash.h"
#include "mupdf/fitz/pool.h"
#include "mupdf/fitz/tree.h"
#include "mupdf/fitz/bidi.h"
#include "mupdf/fitz/xml.h"
#include "mupdf/fitz/compress.h"
#include "mupdf/fitz/store.h"
#include "mupdf/fitz/filter.h"
#include "mupdf/fitz/compressed-buffer.h"
#include "mupdf/fitz/archive.h"
/* Resources */
#include "mupdf/fitz/color.h"
#include "mupdf/fitz/separation.h"
#include "mupdf/fitz/pixmap.h"
#include "mupdf/fitz/bitmap.h"
#include "mupdf/fitz/image.h"
#include "mupdf/fitz/shade.h"
#include "mupdf/fitz/font.h"
#include "mupdf/fitz/path.h"
#include "mupdf/fitz/text.h"
#include "mupdf/fitz/glyph.h"

#include "mupdf/fitz/device.h"
#include "mupdf/fitz/display-list.h"
#include "mupdf/fitz/structured-text.h"
#include "mupdf/fitz/transition.h"
#include "mupdf/fitz/glyph-cache.h"

/* Document */
#include "mupdf/fitz/link.h"
#include "mupdf/fitz/outline.h"
#include "mupdf/fitz/document.h"
#include "mupdf/fitz/util.h"

/* Output formats */
#include "mupdf/fitz/writer.h"
#include "mupdf/fitz/band-writer.h"
#include "mupdf/fitz/write-pixmap.h"
#include "mupdf/fitz/output-svg.h"
//
//#include "mupdf/pdf.h"
#include "mupdf/pdf/object.h"
#include "mupdf/pdf/document.h"
#include "mupdf/pdf/parse.h"
#include "mupdf/pdf/xref.h"
#include "mupdf/pdf/crypt.h"
#include "mupdf/pdf/cmap.h"
#include "mupdf/pdf/font.h"
#include "mupdf/pdf/resource.h"
#include "mupdf/pdf/interpret.h"
#include "mupdf/pdf/page.h"
#include "mupdf/pdf/annot.h"
#include "mupdf/pdf/form.h"
#include "mupdf/pdf/event.h"
#include "mupdf/pdf/javascript.h"
#include "mupdf/pdf/output-pdf.h"
#include "mupdf/pdf/clean.h"
//
#ifdef __cplusplus
// @sobolev }
#endif

#endif
