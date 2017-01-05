/* libsvg - Library for parsing/rendering SVG documents

   Copyright © 2002 USC/Information Sciences Institute

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Carl Worth <cworth@isi.edu>
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <dir.h>
#include <setjmp.h>
#include <stdarg.h>
#include <slib.h>
#include <zlib.h>
#include <libpng\png.h>
#include <jpeglib.h>
#include <jerror.h>
//
// svgint {
//
#include <libxml/SAX.h>
#include <libxml/xmlmemory.h>
#include <libxml/hash.h>

typedef xmlParserCtxtPtr svg_xml_parser_context_t;
typedef xmlHashTable svg_xml_hash_table_t;

#define _svg_xml_hash_add_entry         xmlHashAddEntry
#define _svg_xml_hash_lookup            xmlHashLookup
#define _svg_xml_hash_create            xmlHashCreate
#define _svg_xml_hash_free(_table)      xmlHashFree(_table, NULL)

#include "svg_version.h"
#include "svg.h"
/*
        svg_ascii
 */
/*
        Functions like the ones in <ctype.h> that are not affected by locale.
 */
enum svg_ascii_type_t {
	SVG_ASCII_ALNUM  = 1<<0,
	SVG_ASCII_ALPHA  = 1<<1,
	SVG_ASCII_CNTRL  = 1<<2,
	SVG_ASCII_DIGIT  = 1<<3,
	SVG_ASCII_GRAPH  = 1<<4,
	SVG_ASCII_LOWER  = 1<<5,
	SVG_ASCII_PRINT  = 1<<6,
	SVG_ASCII_PUNCT  = 1<<7,
	SVG_ASCII_SPACE  = 1<<8,
	SVG_ASCII_UPPER  = 1<<9,
	SVG_ASCII_XDIGIT = 1<<10
};

extern const uint16 * const svg_ascii_table;

#define _svg_ascii_isdigit(c) ((svg_ascii_table[(unsigned char)(c)]&SVG_ASCII_DIGIT) != 0)
#define _svg_ascii_isspace(c) ((svg_ascii_table[(unsigned char)(c)]&SVG_ASCII_SPACE) != 0)
#define _svg_ascii_isxdigit(c) ((svg_ascii_table[(unsigned char)(c)]&SVG_ASCII_XDIGIT) != 0)
double _svg_ascii_strtod(const char * nptr, const char ** endptr);
/*
        sure wish C had a real enum so that this type would be
        distinguishable from svg_status_t. In the meantime, we'll use the
        otherwise bogus 1000 value.
 */
enum svgint_status_t {
	SVGINT_STATUS_BROKEN_IMPLEMENTATION = 1000,
	SVGINT_STATUS_ARGS_EXHAUSTED,
	SVGINT_STATUS_UNKNOWN_ELEMENT,
	SVGINT_STATUS_ATTRIBUTE_NOT_FOUND,
	SVGINT_STATUS_IMAGE_NOT_PNG,
	SVGINT_STATUS_IMAGE_NOT_JPEG,
	SVGINT_STATUS_UNDEFINED_RESULT
};
/*
        svg_length.c
 */
svg_status_t _svg_length_init_from_str(USize * length, const char * str);
/*
        svg_attribute.c
*/
svgint_status_t _svg_attribute_get_double(const char ** attributes, const char * name, double * value, double default_value);
svgint_status_t _svg_attribute_get_string(const char ** attributes, const char * name, const char ** value, const char * default_value);
svgint_status_t _svg_attribute_get_length(const char ** attributes, const char * name, USize * value, const char * default_value);

_XmlAttributes::_XmlAttributes(const char ** ppAttrList)
{
	PP_List = ppAttrList;
}

_XmlAttributes::operator const char ** ()
{
	return PP_List;
}

int _XmlAttributes::GetStr(const char * pName, const char * pDef, SString & rBuf)
{
	const char * p_val = Find(pName);
	if(p_val) {
		rBuf = p_val;
		return 1;
	}
	else {
		rBuf = pDef;
		return -1;
	}
}

int _XmlAttributes::GetReal(const char * pName, double def, double & rVal)
{
	const char * p_val = Find(pName);
	if(p_val) {
		rVal = _svg_ascii_strtod(p_val, NULL);
		return 1;
	}
	else {
		rVal = def;
		return -1;
	}
}

int _XmlAttributes::GetUSize(const char * pName, const char * pDef, USize & rVal)
{
	const char * p_val = Find(pName);
	if(p_val)
		return _svg_length_init_from_str(&rVal, p_val) ? 1 : 0;
	else
		return _svg_length_init_from_str(&rVal, pDef) ? -1 : 0;
}

const char * _XmlAttributes::Find(const char * pName)
{
	if(PP_List)
		for(int i = 0; PP_List[i]; i += 2)
			if(strcmp(PP_List[i], pName) == 0)
				return PP_List[i+1];
	return 0;
}

struct svg_path_t : public SDrawPath {
	svg_status_t Init()
	{
		SDrawPath::Clear();
		return SVG_STATUS_SUCCESS;
	}
	svg_status_t ApplyAttributes(const char ** attributes)
	{
		svg_status_t status = SVG_STATUS_SUCCESS;
		const char * path_str;
		if(IsEmpty()) {
			_svg_attribute_get_string(attributes, "d", &path_str, NULL);
			/* XXX: Need to check spec. for this error case */
			if(path_str == NULL)
				status = SVG_STATUS_PARSE_ERROR;
			else if(!FromStr(path_str, SDrawPath::fmtSVG))
				status = SVG_STATUS_PARSE_ERROR;
		}
		return status;
	}
	svg_status_t Render(svg_render_engine_t * engine, void * closure);
};

#define SVG_STYLE_FLAG_NONE                             0x00000000000ULL
#define SVG_STYLE_FLAG_CLIP_RULE                        0x00000000001ULL
#define SVG_STYLE_FLAG_COLOR                            0x00000000002ULL
#define SVG_STYLE_FLAG_COLOR_INTERPOLATION              0x00000000004ULL
#define SVG_STYLE_FLAG_COLOR_INTERPOLATION_FILTERS      0x00000000008ULL
#define SVG_STYLE_FLAG_COLOR_PROFILE                    0x00000000010ULL
#define SVG_STYLE_FLAG_COLOR_RENDERING                  0x00000000020ULL
#define SVG_STYLE_FLAG_CURSOR                           0x00000000040ULL
#define SVG_STYLE_FLAG_DIRECTION                        0x00000000080ULL
#define SVG_STYLE_FLAG_DISPLAY                          0x00000000100ULL
#define SVG_STYLE_FLAG_FILL_OPACITY                     0x00000000200ULL
#define SVG_STYLE_FLAG_FILL_PAINT                       0x00000000400ULL
#define SVG_STYLE_FLAG_FILL_RULE                        0x00000000800ULL
#define SVG_STYLE_FLAG_FONT_FAMILY                      0x00000001000ULL
#define SVG_STYLE_FLAG_FONT_SIZE                        0x00000002000ULL
#define SVG_STYLE_FLAG_FONT_SIZE_ADJUST                 0x00000004000ULL
#define SVG_STYLE_FLAG_FONT_STRETCH                     0x00000008000ULL
#define SVG_STYLE_FLAG_FONT_STYLE                       0x00000010000ULL
#define SVG_STYLE_FLAG_FONT_VARIANT                     0x00000020000ULL
#define SVG_STYLE_FLAG_FONT_WEIGHT                      0x00000040000ULL
#define SVG_STYLE_FLAG_GLYPH_ORIENTATION_HORIZONTAL     0x00000080000ULL
#define SVG_STYLE_FLAG_GLYPH_ORIENTATION_VERTICAL       0x00000100000ULL
#define SVG_STYLE_FLAG_IMAGE_RENDERING                  0x00000200000ULL
#define SVG_STYLE_FLAG_KERNING                          0x00000400000ULL
#define SVG_STYLE_FLAG_LETTER_SPACING                   0x00000800000ULL
#define SVG_STYLE_FLAG_MARKER_END                       0x00001000000ULL
#define SVG_STYLE_FLAG_MARKER_MID                       0x00002000000ULL
#define SVG_STYLE_FLAG_MARKER_START                     0x00004000000ULL
#define SVG_STYLE_FLAG_OPACITY                          0x00008000000ULL
#define SVG_STYLE_FLAG_POINTER_EVENTS                   0x00010000000ULL
#define SVG_STYLE_FLAG_SHAPE_RENDERING                  0x00020000000ULL
#define SVG_STYLE_FLAG_STROKE_DASH_ARRAY                0x00040000000ULL
#define SVG_STYLE_FLAG_STROKE_DASH_OFFSET               0x00080000000ULL
#define SVG_STYLE_FLAG_STROKE_LINE_CAP                  0x00100000000ULL
#define SVG_STYLE_FLAG_STROKE_LINE_JOIN                 0x00200000000ULL
#define SVG_STYLE_FLAG_STROKE_MITER_LIMIT               0x00400000000ULL
#define SVG_STYLE_FLAG_STROKE_OPACITY                   0x00800000000ULL
#define SVG_STYLE_FLAG_STROKE_PAINT                     0x01000000000ULL
#define SVG_STYLE_FLAG_STROKE_WIDTH                     0x02000000000ULL
#define SVG_STYLE_FLAG_TEXT_ANCHOR                      0x04000000000ULL
#define SVG_STYLE_FLAG_TEXT_RENDERING                   0x08000000000ULL
#define SVG_STYLE_FLAG_VISIBILITY                       0x10000000000ULL
#define SVG_STYLE_FLAG_WORD_SPACING                     0x20000000000ULL
#define SVG_STYLE_FLAG_WRITING_MODE                     0x40000000000ULL

struct svg_style_t {
	svg_status_t Init(svg_t * pSvg)
	{
		svg = pSvg;
		flags = SVG_STYLE_FLAG_NONE;
		font_family = NULL;
		font_size.Set(10, UNIT_GR_PIXEL);
		num_dashes = 0;
		stroke_dash_array = NULL;
		stroke_dash_offset.S = 0;
		/* initialize unused elements so copies are predictable */
		stroke_line_cap = SVG_STROKE_LINE_CAP_BUTT;
		stroke_line_join = SVG_STROKE_LINE_JOIN_MITER;
		stroke_miter_limit = 4.0;
		stroke_opacity = 1.0;
		fill_opacity = 1.0;
		/* opacity is not inherited */
		flags |= SVG_STYLE_FLAG_OPACITY;
		opacity = 1.0;
		flags |= SVG_STYLE_FLAG_VISIBILITY;
		flags |= SVG_STYLE_FLAG_DISPLAY;
		return SVG_STATUS_SUCCESS;
	}
	svg_status_t Init(svg_style_t * other)
	{
		svg = other->svg;
		flags = other->flags;
		fill_opacity = other->fill_opacity;
		fill_paint = other->fill_paint;
		fill_rule = other->fill_rule;
		if(other->font_family) {
			font_family = strdup(other->font_family);
			if(font_family == NULL)
				return SVG_STATUS_NO_MEMORY;
		}
		else
			font_family = NULL;
		font_size = other->font_size;
		font_style = other->font_style;
		font_weight = other->font_weight;
		opacity = other->opacity;
		num_dashes = other->num_dashes;
		if(num_dashes) {
			stroke_dash_array = (double *)malloc(num_dashes*sizeof(double));
			if(stroke_dash_array == NULL)
				return SVG_STATUS_NO_MEMORY;
			memcpy(stroke_dash_array, other->stroke_dash_array, num_dashes*sizeof(double));
		}
		else
			stroke_dash_array = NULL;
		stroke_dash_offset = other->stroke_dash_offset;
		stroke_line_cap = other->stroke_line_cap;
		stroke_line_join = other->stroke_line_join;
		stroke_miter_limit = other->stroke_miter_limit;
		stroke_opacity = other->stroke_opacity;
		stroke_paint = other->stroke_paint;
		stroke_width = other->stroke_width;
		color = other->color;
		text_anchor = other->text_anchor;
		return SVG_STATUS_SUCCESS;
	}
	svg_status_t Destroy()
	{
		ZFREE(font_family);
		ZFREE(stroke_dash_array);
		num_dashes = 0;
		flags = SVG_STYLE_FLAG_NONE;
		return SVG_STATUS_SUCCESS;
	}
	svg_t * svg;
	uint64 flags;
	double fill_opacity;
	svg_paint_t fill_paint;
	svg_fill_rule_t fill_rule;
	char * font_family;
	USize font_size;
	svg_font_style_t font_style;
	uint font_weight;
	double opacity;
	double * stroke_dash_array;
	int num_dashes;
	USize stroke_dash_offset;
	svg_stroke_line_cap_t stroke_line_cap;
	svg_stroke_line_join_t stroke_line_join;
	double stroke_miter_limit;
	double stroke_opacity;
	svg_paint_t stroke_paint;
	USize stroke_width;
	svg_color_t color;
	svg_text_anchor_t text_anchor;
};

/*
struct LMatrix2D {
	double m[3][2];
};
*/

struct svg_group {
	svg_element_t ** element;
	int num_elements;
	int element_size;
	USize width;
	USize height;
	svg_view_box_t view_box;
	USize x;
	USize y;
};

struct svg_image_t {
	char * url;
	char * data;
	uint data_width;
	uint data_height;
	/* User-space position and size */
	USize x;
	USize y;
	USize width;
	USize height;
};

struct svg_element {
	svg_status_t render(svg_render_engine_t * engine, void * closure);

	struct Ellipse_ {
		void Init()
		{
			cx.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			cy.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
			rx.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			ry.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
		}
		USize cx;
		USize cy;
		USize rx;
		USize ry;
	};
	struct Line_ {
		void Init()
		{
			x1.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			y1.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
			x2.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			y2.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
		}
		USize x1;
		USize y1;
		USize x2;
		USize y2;
	};
	struct Rect_ {
		void Init()
		{
			x.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			y.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
			width.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			height.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
			rx.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			ry.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
		}
		USize  x;
		USize  y;
		USize  width;
		USize  height;
		USize  rx;
		USize  ry;
	};
	struct Text_ {
		void Init()
		{
			x.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
			y.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
		}
		USize  x;
		USize  y;
	};
	svg_element * P_Parent;
	svg_t * P_Doc;
	LMatrix2D transform;
	svg_style_t style;
	svg_element_type_t type;
	SString Ident;
	SString Txt;

	SvgGradient gradient;
	svg_pattern_t pattern;
	svg_path_t path;
	svg_group_t group;

	union {
		Ellipse_ ellipse;
		Line_ line;
		Rect_ rect;
		Text_ text;
		svg_image_t image;
		SvgGradientStop gradient_stop;
	} e;
};

extern svg_t * doc;
/*
        svg.c
 */
svg_status_t _svg_fetch_element_by_id(svg_t * svg, const char * id, svg_element_t ** element_ret);
/*
        libsvg_features.c
 */
extern const uint libsvg_major_version, libsvg_minor_version, libsvg_micro_version;
extern const char * libsvg_version;

void libsvg_preinit(void * app, void * modinfo);
void libsvg_postinit(void * app, void * modinfo);
/*
        svg_color.c
 */
svg_status_t _svg_color_init_from_str(svg_color_t * color, const char * str);
/*
        svg_element.c
 */
svgint_status_t _svg_element_create(svg_element_t ** element, svg_element_type_t type, svg_element_t * parent, svg_t * doc);
svg_status_t _svg_element_init_copy(svg_element_t * element, svg_element_t * other);
svg_status_t _svg_element_destroy(svg_element_t * element);
svgint_status_t _svg_element_clone(svg_element_t ** element, svg_element_t * other);
svg_status_t _svg_element_apply_attributes(svg_element_t * group_element, _XmlAttributes & rAttrList);
svg_status_t _svg_element_get_nearest_viewport(svg_element_t * element, svg_element_t ** viewport);
/*
        svg_group.c
 */
svg_status_t _svg_group_init(svg_group_t * group);
svg_status_t _svg_group_init_copy(svg_group_t * group, svg_group_t * other);
svg_status_t _svg_group_add_element(svg_group_t * group, svg_element_t * element);
svg_status_t _svg_group_render(svg_group_t * group, svg_render_engine_t * engine, void * closure);
svg_status_t _svg_symbol_render(svg_element_t * group, svg_render_engine_t * engine, void * closure);
svg_status_t _svg_group_apply_svg_attributes(svg_group_t * group, _XmlAttributes & rAttrList);
svg_status_t _svg_group_apply_group_attributes(svg_group_t * group, const char ** attributes);
svg_status_t _svg_group_apply_use_attributes(svg_element_t * group, const char ** attributes);
svg_status_t _svg_group_get_size(svg_group_t * group, USize * width, USize * height);
/*
        svg_image.c
 */
svg_status_t _svg_image_init(svg_image_t * image);
svg_status_t _svg_image_init_copy(svg_image_t * image, svg_image_t * other);
svg_status_t _svg_image_deinit(svg_image_t * image);
svg_status_t _svg_image_apply_attributes(svg_image_t * image, const char ** attributes);
svg_status_t _svg_image_render(svg_image_t * image, svg_render_engine_t * engine, void * closure);
/*
        svg_paint.c
 */
svg_status_t _svg_paint_init(svg_paint_t * paint, svg_t * svg, const char * str);
svg_status_t _svg_paint_deinit(svg_paint_t * paint);
/*
        svg_parser.c
 */
void _svg_parser_sax_start_element(void * closure, const xmlChar * name, const xmlChar ** atts);
void _svg_parser_sax_end_element(void * closure, const xmlChar * name);
void _svg_parser_sax_characters(void * closure, const xmlChar * ch, int len);
/*
        svg_path.c
 */
svg_status_t _svg_path_render(svg_path_t * path, svg_render_engine_t * engine, void * closure);
svg_status_t _svg_path_close_path(svg_path_t * path);
/*
        svg_pattern.c
 */
svg_status_t _svg_pattern_init(svg_pattern_t * pattern, svg_element_t * parent, svg_t * doc);
svg_status_t _svg_pattern_init_copy(svg_pattern_t * pattern, svg_pattern_t * other);
svg_status_t _svg_pattern_deinit(svg_pattern_t * pattern);
svg_status_t _svg_pattern_apply_attributes(svg_pattern_t * pattern, const char ** attributes);
svg_status_t _svg_pattern_render(svg_element_t * pattern, svg_render_engine_t * engine, void * closure);
/*
        svg_str.c
 */
void _svg_str_skip_space(const char ** str);
void _svg_str_skip_char(const char ** str, char c);
void _svg_str_skip_space_or_char(const char ** str, char c);
svgint_status_t _svg_str_parse_csv_doubles(const char * str, double * value, int num_values, const char ** end);
svgint_status_t _svg_str_parse_all_csv_doubles(const char * str, double ** value, int * num_values, const char ** end);
/*
        svg_style.c
 */
svg_status_t _svg_style_init_defaults(svg_style_t * style, svg_t * svg);
svg_status_t _svg_style_render(svg_style_t * style, svg_render_engine_t * engine, void * closure);
svg_status_t _svg_style_apply_attributes(svg_style_t * style, const char ** attributes);
svg_status_t _svg_style_get_display(svg_style_t * style);
svg_status_t _svg_style_get_visibility(svg_style_t * style);
//
// } svgint
//
#ifndef MAXPATH
 #define MAXPATH 260
#endif

svg_t::svg_t()
{
	dpi = 100;
	dir_name = strdup(".");
	group_element = NULL;
	parser.svg = this;
	parser.ctxt = NULL;
	parser.unknown_element_depth = 0;
	parser.state = NULL;
	parser.status = SVG_STATUS_SUCCESS;
	engine = NULL;
	element_ids = _svg_xml_hash_create(100);
}

svg_t::~svg_t()
{
	ZFREE(dir_name);
	if(group_element)
		_svg_element_destroy(group_element);
	parser.svg = NULL;
	parser.ctxt = NULL;
	parser.status = SVG_STATUS_SUCCESS;
	engine = NULL;
	_svg_xml_hash_free((svg_xml_hash_table_t *)element_ids);
}

#define SVG_PARSE_BUFFER_SIZE (8*1024)

svg_status_t svg_parse_file(svg_t * svg, FILE * file)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
	/* @sobolev Времмено закомментировано из-за dup()
	   char buf[SVG_PARSE_BUFFER_SIZE];
	   int read;
	   gzFile zfile = gzdopen(dup(fileno(file)), "r");
	   if(zfile == NULL) {
	        switch(errno) {
	            case ENOMEM:
	                return SVG_STATUS_NO_MEMORY;
	            case ENOENT:
	                return SVG_STATUS_FILE_NOT_FOUND;
	            default:
	                return SVG_STATUS_IO_ERROR;
	        }
	   }
	   status = svg_parse_chunk_begin(svg);
	   if(status)
	        goto CLEANUP;
	   while(!gzeof(zfile)) {
	        read = gzread(zfile, buf, SVG_PARSE_BUFFER_SIZE);
	        if(read > -1) {
	                status = svg_parse_chunk(svg, buf, read);
	                if(status)
	                        goto CLEANUP;
	        }
	        else {
	                status = SVG_STATUS_IO_ERROR;
	                goto CLEANUP;
	        }
	   }
	   status = svg_parse_chunk_end(svg);
	   CLEANUP:
	   gzclose(zfile);
	 */
	return status;
}

svg_status_t svg_parse(svg_t * svg, const char * filename)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
#if 0 // @sobolev Временно закомментировано из-за dirname {
	FILE * file;
	char * tmp;
	free(svg->dir_name);
	/* awful dirname semantics require some hoops */
	tmp = strdup(filename);
	svg->dir_name = strdup(dirname(tmp));
	free(tmp);
	file = fopen(filename, "r");
	if(file == NULL) {
		switch(errno) {
		    case ENOMEM: return SVG_STATUS_NO_MEMORY;
		    case ENOENT: return SVG_STATUS_FILE_NOT_FOUND;
		    default: return SVG_STATUS_IO_ERROR;
		}
	}
	status = svg_parse_file(svg, file);
	fclose(file);
#endif // } 0
	return status;
}

svg_status_t svg_parse_buffer(svg_t * svg, const char * buf, size_t count)
{
	svg_status_t status = svg_parse_chunk_begin(svg);
	if(status)
		return status;
	status = svg_parse_chunk(svg, buf, count);
	return status ? status : svg_parse_chunk_end(svg);
}

svg_status_t svg_parse_chunk(svg_t * svg, const char * buf, size_t count)
{
	if(svg->parser.status)
		return svg->parser.status;
	if(svg->parser.ctxt == NULL)
		return SVG_STATUS_INVALID_CALL;
	xmlParseChunk((svg_xml_parser_context_t)svg->parser.ctxt, buf, count, 0);
	return svg->parser.status;
}

svg_status_t svg_parse_chunk_end(svg_t * svg)
{
	if(svg->parser.ctxt == NULL)
		return SVG_STATUS_INVALID_CALL;
	if(!((svg_xml_parser_context_t)svg->parser.ctxt)->wellFormed)
		svg->parser.status = SVG_STATUS_PARSE_ERROR;
	xmlFreeParserCtxt((svg_xml_parser_context_t)svg->parser.ctxt);
	svg->parser.ctxt = NULL;
	xmlHashFree((svg_xml_hash_table_t *)svg->parser.entities, (xmlHashDeallocator)xmlFree);
	svg->parser.entities = NULL;
	return svg->parser.status;
}

svg_status_t svg_render(svg_t * svg, svg_render_engine_t * engine, void * closure)
{
	svg_status_t status;
	char orig_dir[MAXPATH];
	if(svg->group_element == NULL)
		return SVG_STATUS_SUCCESS;
	/* XXX: Currently, the SVG parser doesn't resolve relative URLs
	   properly, so I'll just cheese things in by changing the current
	   directory -- at least I'll be nice about it and restore it
	   afterwards. */
	getcwd(orig_dir, MAXPATH);
	chdir(svg->dir_name);
	status = svg->group_element->render(engine, closure);
	chdir(orig_dir);
	return status;
}

svg_status_t _svg_fetch_element_by_id(svg_t * svg, const char * id, svg_element_t ** element_ret)
{
	*element_ret = (svg_element_t *)_svg_xml_hash_lookup((svg_xml_hash_table_t *)svg->element_ids, (uchar *)id);
	return SVG_STATUS_SUCCESS;
}

void svg_get_size(svg_t * svg, USize * width, USize * height)
{
	if(svg->group_element) {
		_svg_group_get_size(&svg->group_element->group, width, height);
	}
	else {
		width->Set(0.0, UNIT_GR_PIXEL, DIREC_UNKN);
		height->Set(0.0, UNIT_GR_PIXEL, DIREC_UNKN);
	}
}
//
// svg_length
//
svg_status_t _svg_length_init_from_str(USize * length, const char * str)
{
	int    prev_dir = length->Dir;
	int    r = length->FromStr(str, USize::fmtSVG);
	if(!r)
		return SVG_STATUS_PARSE_ERROR;
	else {
		SETIFZ(length->Unit, UNIT_GR_PIXEL);
		SETIFZ(length->Dir, prev_dir);
	}
	return SVG_STATUS_SUCCESS;
}
//
// svg_attribute
//
svgint_status_t _svg_attribute_get_double(const char ** attributes, const char * name, double * value, double default_value)
{
	*value = default_value;
	if(attributes)
		for(int i = 0; attributes[i]; i += 2)
			if(strcmp(attributes[i], name) == 0) {
				*value = _svg_ascii_strtod(attributes[i+1], NULL);
				return (svgint_status_t)SVG_STATUS_SUCCESS;
			}
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}

svgint_status_t _svg_attribute_get_string(const char ** attributes, const char * name, const char ** value, const char * default_value)
{
	*value = default_value;
	if(attributes)
		for(int i = 0; attributes[i]; i += 2)
			if(strcmp(attributes[i], name) == 0) {
				*value = attributes[i+1];
				return (svgint_status_t)SVG_STATUS_SUCCESS;
			}
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}

svgint_status_t _svg_attribute_get_length(const char ** attributes, const char * name, USize * value, const char * default_value)
{
	_svg_length_init_from_str(value, default_value);
	if(attributes)
		for(int i = 0; attributes[i]; i += 2)
			if(strcmp(attributes[i], name) == 0) {
				_svg_length_init_from_str(value, attributes[i+1]);
				return (svgint_status_t)SVG_STATUS_SUCCESS;
			}
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}
//
// svg_color
//
svg_status_t _svg_color_init_from_str(svg_color_t * color, const char * str)
{
	return color->FromStr(str) ? SVG_STATUS_SUCCESS : SVG_STATUS_PARSE_ERROR;
}
//
// svg_element
//
svgint_status_t _svg_element_create(svg_element_t ** ppElement, svg_element_type_t type, svg_element_t * parent, svg_t * doc)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
	svg_element_t * p_elem = new svg_element_t;
	*ppElement = p_elem;
	if(p_elem == NULL)
		status = SVG_STATUS_NO_MEMORY;
	else {
		p_elem->type = type;
		p_elem->P_Parent = parent;
		p_elem->P_Doc = doc;
		p_elem->Ident = 0;
		p_elem->transform.InitUnit();
		status = p_elem->style.Init(doc);
		if(status == 0) {
			switch(type) {
				case SVG_ELEMENT_TYPE_SVG_GROUP:
				case SVG_ELEMENT_TYPE_GROUP:
				case SVG_ELEMENT_TYPE_DEFS:
				case SVG_ELEMENT_TYPE_USE:
				case SVG_ELEMENT_TYPE_SYMBOL: status = _svg_group_init(&p_elem->group); break;
				case SVG_ELEMENT_TYPE_PATH: p_elem->path.Init(); break;
				case SVG_ELEMENT_TYPE_CIRCLE:
				case SVG_ELEMENT_TYPE_ELLIPSE: p_elem->e.ellipse.Init(); break;
				case SVG_ELEMENT_TYPE_LINE: p_elem->e.line.Init(); break;
				case SVG_ELEMENT_TYPE_RECT: p_elem->e.rect.Init(); break;
				case SVG_ELEMENT_TYPE_TEXT:
					p_elem->e.text.Init();
					p_elem->Txt = 0;
					break;
				case SVG_ELEMENT_TYPE_IMAGE: status = _svg_image_init(&p_elem->e.image); break;
				case SVG_ELEMENT_TYPE_GRADIENT: status = p_elem->gradient.Init(); break;
				case SVG_ELEMENT_TYPE_PATTERN: status = _svg_pattern_init(&p_elem->pattern, parent, doc); break;
				default: status = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT; break;
			}
		}
	}
	return (svgint_status_t)status;
}

svg_status_t _svg_element_init_copy(svg_element_t * element, svg_element_t * other)
{
	element->type   = other->type;
	element->P_Parent = other->P_Parent;
	element->Ident = other->Ident;
	element->transform = other->transform;
	svg_status_t status = element->style.Init(&other->style);
	if(status)
		return status;
	switch(other->type) {
	    case SVG_ELEMENT_TYPE_SVG_GROUP:
	    case SVG_ELEMENT_TYPE_GROUP:
	    case SVG_ELEMENT_TYPE_DEFS:
	    case SVG_ELEMENT_TYPE_USE:
	    case SVG_ELEMENT_TYPE_SYMBOL: status = _svg_group_init_copy(&element->group, &other->group); break;
	    case SVG_ELEMENT_TYPE_PATH: element->path.Copy(other->path); break;
	    case SVG_ELEMENT_TYPE_CIRCLE:
	    case SVG_ELEMENT_TYPE_ELLIPSE: element->e.ellipse = other->e.ellipse; break;
	    case SVG_ELEMENT_TYPE_LINE: element->e.line = other->e.line; break;
	    case SVG_ELEMENT_TYPE_RECT: element->e.rect = other->e.rect; break;
	    case SVG_ELEMENT_TYPE_TEXT:
			element->e.text = other->e.text;
			element->Txt = other->Txt;
			break;
	    case SVG_ELEMENT_TYPE_GRADIENT: status = element->gradient.Init(&other->gradient); break;
	    case SVG_ELEMENT_TYPE_PATTERN: status = _svg_pattern_init_copy(&element->pattern, &other->pattern); break;
	    case SVG_ELEMENT_TYPE_IMAGE: status = _svg_image_init_copy(&element->e.image, &other->e.image); break;
	    default: status = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT; break;
	}
	return status ? status : SVG_STATUS_SUCCESS;
}

svgint_status_t _svg_element_clone(svg_element_t ** element, svg_element_t * other)
{
	*element = (svg_element_t *)malloc(sizeof(svg_element_t));
	return (*element == NULL) ? (svgint_status_t)SVG_STATUS_NO_MEMORY : (svgint_status_t)_svg_element_init_copy(*element, other);
}

svg_status_t _svg_element_destroy(svg_element_t * element)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
	element->transform.InitUnit();
	element->style.Destroy();
	element->Ident = 0;
	switch(element->type) {
		case SVG_ELEMENT_TYPE_SVG_GROUP:
		case SVG_ELEMENT_TYPE_GROUP:
		case SVG_ELEMENT_TYPE_DEFS:
		case SVG_ELEMENT_TYPE_USE:
		case SVG_ELEMENT_TYPE_SYMBOL:
			{
				for(int i = 0; i < element->group.num_elements; i++)
					_svg_element_destroy(element->group.element[i]);
				free(element->group.element);
				element->group.element = NULL;
				element->group.num_elements = 0;
				element->group.element_size = 0;
			}
			break;
		case SVG_ELEMENT_TYPE_PATH: element->path.Clear(); break;
		case SVG_ELEMENT_TYPE_CIRCLE:
		case SVG_ELEMENT_TYPE_ELLIPSE:
		case SVG_ELEMENT_TYPE_LINE:
		case SVG_ELEMENT_TYPE_RECT: status = SVG_STATUS_SUCCESS; break;
		case SVG_ELEMENT_TYPE_TEXT: element->Txt = 0; break;
		case SVG_ELEMENT_TYPE_GRADIENT: status = element->gradient.Deinit(); break;
		case SVG_ELEMENT_TYPE_PATTERN: status = _svg_pattern_deinit(&element->pattern); break;
		case SVG_ELEMENT_TYPE_IMAGE: status = _svg_image_deinit(&element->e.image); break;
		default: status = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT; break;
	}
	delete element;
	return status;
}

svg_status_t svg_element::render(svg_render_engine_t * engine, void * closure)
{
	svg_status_t return_status = SVG_STATUS_SUCCESS;
	/* if the display property is not activated, we dont have to
	   draw this element nor its children, so we can safely return here. */
	svg_status_t status = _svg_style_get_display(&style);
	if(status)
		return status;
	if(oneof2(type, SVG_ELEMENT_TYPE_SVG_GROUP, SVG_ELEMENT_TYPE_GROUP)) {
		status = (engine->begin_group)(closure, style.opacity);
		if(status)
			return status;
	}
	else {
		status = (engine->begin_element)(closure);
		if(status)
			return status;
	}
	if(type == SVG_ELEMENT_TYPE_SVG_GROUP) {
		status = (engine->set_viewport_dimension)(closure, &group.width, &group.height);
		if(status)
			return status;
	}
	/* perform extra viewBox transform */
	if(oneof2(type, SVG_ELEMENT_TYPE_SVG_GROUP, SVG_ELEMENT_TYPE_GROUP) && group.view_box.AspectRatio != svg_view_box_t::pasUnkn) {
		status = (engine->apply_view_box)(closure, group.view_box, &group.width, &group.height);
	}
	/* TODO : this is probably not the right place to change transform, but
	   atm we dont store USize in group, so... */
	if(oneof2(type, SVG_ELEMENT_TYPE_SVG_GROUP, SVG_ELEMENT_TYPE_USE)) {
		LMatrix2D translate;
		transform = translate.InitTranslate(group.x, group.y) * transform;
	}
	{
		status = SVG_STATUS_SUCCESS; // @sobolev temp
		/* @sobolev temp
		status = (engine->transform)(closure,
			transform.m[0][0], transform.m[0][1],
			transform.m[1][0], transform.m[1][1],
			transform.m[2][0], transform.m[2][1]);
		*/
	}
	if(status)
		return status;
	status = _svg_style_render(&style, engine, closure);
	if(status)
		return status;
	/* If the element doesnt have children, we can check visibility property, otherwise
	   the children will have to be processed. */
	if(type != SVG_ELEMENT_TYPE_SVG_GROUP && type != SVG_ELEMENT_TYPE_GROUP && type != SVG_ELEMENT_TYPE_USE)
		return_status = _svg_style_get_visibility(&style);
	if(return_status == SVG_STATUS_SUCCESS) {
		switch(type) {
		    case SVG_ELEMENT_TYPE_SVG_GROUP:
		    case SVG_ELEMENT_TYPE_GROUP:
		    case SVG_ELEMENT_TYPE_USE: status = _svg_group_render(&group, engine, closure); break;
		    case SVG_ELEMENT_TYPE_PATH: status = _svg_path_render(&path, engine, closure); break;
		    case SVG_ELEMENT_TYPE_CIRCLE:
				status = (e.ellipse.rx == 0.0) ? SVG_STATUS_SUCCESS : (engine->render_ellipse)(closure, &e.ellipse.cx, &e.ellipse.cy, &e.ellipse.rx, &e.ellipse.rx);
				break;
		    case SVG_ELEMENT_TYPE_ELLIPSE:
				status = (e.ellipse.rx == 0.0 || e.ellipse.ry == 0.0) ? SVG_STATUS_SUCCESS :
					(engine->render_ellipse)(closure, &e.ellipse.cx, &e.ellipse.cy, &e.ellipse.rx, &e.ellipse.ry);
				break;
		    case SVG_ELEMENT_TYPE_LINE:
				status = (engine->render_line)(closure, &e.line.x1, &e.line.y1, &e.line.x2, &e.line.y2);
				break;
		    case SVG_ELEMENT_TYPE_RECT:
				status = (engine->render_rect)(closure, &e.rect.x, &e.rect.y, &e.rect.width, &e.rect.height, &e.rect.rx, &e.rect.ry);
				break;
		    case SVG_ELEMENT_TYPE_TEXT:
				//status = _svg_text_render(&e.text, engine, closure);
				//svg_status_t _svg_text_render(svg_text_t * text, svg_render_engine_t * engine, void * closure)
				{
					status = (engine->render_text)(closure, &e.text.x, &e.text.y, (const char *)Txt);
				}
				break;
		    case SVG_ELEMENT_TYPE_IMAGE: status = _svg_image_render(&e.image, engine, closure); break;
		    case SVG_ELEMENT_TYPE_DEFS: break;
		    case SVG_ELEMENT_TYPE_GRADIENT:
				break; /* Gradients are applied as paint, not rendered directly */
		    case SVG_ELEMENT_TYPE_PATTERN:
				break; /* Patterns are applied as paint, not rendered directly */
		    case SVG_ELEMENT_TYPE_SYMBOL: status = _svg_symbol_render(this, engine, closure); break;
		    default: status = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT; break;
		}
		if(status)
			return_status = status;
	}
	if(type == SVG_ELEMENT_TYPE_SVG_GROUP || type == SVG_ELEMENT_TYPE_GROUP) {
		status = (engine->end_group)(closure, style.opacity);
		if(status && !return_status)
			return_status = status;
	}
	else {
		status = (engine->end_element)(closure);
		if(status && !return_status)
			return_status = status;
	}
	return return_status;
}

svg_status_t _svg_element_get_nearest_viewport(svg_element_t * element, svg_element_t ** viewport)
{
	svg_element_t * elem = element;
	*viewport = NULL;
	while(elem && !*viewport) {
		if(elem->type == SVG_ELEMENT_TYPE_SVG_GROUP)
			*viewport = elem;
		elem = elem->P_Parent;
	}
	return SVG_STATUS_SUCCESS;
}

int svg_view_box_t::ParseAspectRatio(const char * pStr)
{
	SStrScan scan(pStr);
	SString ident;
	AspectRatio = pasNone;
	while(scan.Skip().GetIdent(ident)) {
		if(ident.Cmp("xMinYMin", 0) == 0)
			AspectRatio = pasXMINYMIN;
		else if(ident.Cmp("xMidYMin", 0) == 0)
			AspectRatio = pasXMIDYMIN;
		else if(ident.Cmp("xMaxYMin", 0) == 0)
			AspectRatio = pasXMAXYMIN;
		else if(ident.Cmp("xMinYMid", 0) == 0)
			AspectRatio = pasXMINYMID;
		else if(ident.Cmp("xMidYMid", 0) == 0)
			AspectRatio = pasXMIDYMID;
		else if(ident.Cmp("xMaxYMid", 0) == 0)
			AspectRatio = pasXMAXYMID;
		else if(ident.Cmp("xMinYMax", 0) == 0)
			AspectRatio = pasXMINYMAX;
		else if(ident.Cmp("xMidYMax", 0) == 0)
			AspectRatio = pasXMIDYMAX;
		else if(ident.Cmp("xMaxYMax", 0) == 0)
			AspectRatio = pasXMAXYMAX;
		else if(ident.Cmp("meet", 0) == 0)
			MeetOrSlice = mosMeet;
		else if(ident.Cmp("slice", 0) == 0)
			MeetOrSlice = mosSlice;
	}
	return SVG_STATUS_SUCCESS;
}

int svg_view_box_t::ParseBox(const char * pStr)
{
	SStrScan scan(pStr);
	SString nmb_buf;
	THROW(scan.Skip().GetNumber(nmb_buf));
	Box.a.X = (float)nmb_buf.ToReal();
	if(scan.Skip()[0] == ',')
		scan.Incr();
	THROW(scan.Skip().GetNumber(nmb_buf));
	Box.a.Y = (float)nmb_buf.ToReal();
	if(scan.Skip()[0] == ',')
		scan.Incr();
	THROW(scan.Skip().GetNumber(nmb_buf));
	Box.b.X = Box.a.X + (float)nmb_buf.ToReal();
	if(scan.Skip()[0] == ',')
		scan.Incr();
	THROW(scan.Skip().GetNumber(nmb_buf));
	Box.b.Y = Box.a.Y + (float)nmb_buf.ToReal();
	CATCH
		return SVG_STATUS_PARSE_ERROR;
	ENDCATCH
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_element_apply_attributes(svg_element_t * element, _XmlAttributes & rAttrList)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
	SString attr;
	{
		rAttrList.GetStr("transform", 0, attr);
		if(!element->transform.FromStr(attr, LMatrix2D::fmtSVG))
			return SVG_STATUS_PARSE_ERROR;
	}
	status = _svg_style_apply_attributes(&element->style, rAttrList);
	if(status)
		return status;
	rAttrList.GetStr("id", 0, element->Ident);
	switch(element->type) {
	    case SVG_ELEMENT_TYPE_SVG_GROUP:
			status = _svg_group_apply_svg_attributes(&element->group, rAttrList);
			if(status)
				return status;
	    /* fall-through */
	    case SVG_ELEMENT_TYPE_GROUP:
			status = _svg_group_apply_group_attributes(&element->group, rAttrList);
			break;
		case SVG_ELEMENT_TYPE_SYMBOL:
			status = _svg_group_apply_svg_attributes(&element->group, rAttrList);
			break;
	    case SVG_ELEMENT_TYPE_USE:
			status = _svg_group_apply_use_attributes(element, rAttrList);
			break;
	    case SVG_ELEMENT_TYPE_PATH:
			status = element->path.ApplyAttributes(rAttrList);
			break;
	    case SVG_ELEMENT_TYPE_RECT:
	    case SVG_ELEMENT_TYPE_CIRCLE:
	    case SVG_ELEMENT_TYPE_ELLIPSE:
	    case SVG_ELEMENT_TYPE_LINE:
		break;
	    case SVG_ELEMENT_TYPE_TEXT:
			rAttrList.GetUSize("x", "0", element->e.text.x);
			rAttrList.GetUSize("y", "0", element->e.text.y);
			/* XXX: What else goes here? */
			status = SVG_STATUS_SUCCESS;
			break;
	    case SVG_ELEMENT_TYPE_IMAGE:
			status = _svg_image_apply_attributes(&element->e.image, rAttrList);
			break;
	    case SVG_ELEMENT_TYPE_GRADIENT:
			status = element->gradient.ApplyAttributes(element->P_Doc, rAttrList);
			break;
	    case SVG_ELEMENT_TYPE_PATTERN:
			status = _svg_pattern_apply_attributes(&element->pattern, rAttrList);
			break;
	    default:
		status = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT;
		break;
	}
	return status ? status : SVG_STATUS_SUCCESS;
}

svg_pattern_t * svg_element_pattern(svg_element_t * element)
{
	return (element->type != SVG_ELEMENT_TYPE_PATTERN) ? NULL : &element->pattern;
}
//
// svg_gradient
//
svg_status_t SvgGradient::Init()
{
	SetType(SVG_GRADIENT_LINEAR);
	units = SVG_GRADIENT_UNITS_BBOX;
	spread = SVG_GRADIENT_SPREAD_PAD;
	transform.InitUnit();
	stops = NULL;
	num_stops = 0;
	stops_size = 0;
	return SVG_STATUS_SUCCESS;
}

svg_status_t SvgGradient::Init(SvgGradient * other)
{
	*this = *other;
	stops = (SvgGradientStop *)malloc(stops_size*sizeof(SvgGradientStop));
	if(stops == NULL)
		return SVG_STATUS_NO_MEMORY;
	memcpy(stops, other->stops, num_stops*sizeof(SvgGradientStop));
	return SVG_STATUS_SUCCESS;
}

svg_status_t SvgGradient::Deinit()
{
	ZFREE(stops);
	stops_size = 0;
	num_stops = 0;
	return SVG_STATUS_SUCCESS;
}

svg_status_t SvgGradient::SetType(svg_gradient_type_t _type)
{
	type = _type;
	/* XXX: Should check what these defaults should really be. */
	if(type == SVG_GRADIENT_LINEAR) {
		u.linear.x1.Set(0, UNIT_PERCENT, DIREC_HORZ);
		u.linear.y1.Set(0, UNIT_PERCENT, DIREC_VERT);
		u.linear.x2.Set(100, UNIT_PERCENT, DIREC_HORZ);
		u.linear.y2.Set(0, UNIT_PERCENT, DIREC_VERT);
	}
	else {
		u.radial.cx.Set(50, UNIT_PERCENT, DIREC_HORZ);
		u.radial.cy.Set(50, UNIT_PERCENT, DIREC_VERT);
		u.radial.fx.Set(50, UNIT_PERCENT, DIREC_HORZ);
		u.radial.fy.Set(50, UNIT_PERCENT, DIREC_VERT);
		u.radial.r.Set(50, UNIT_PERCENT, DIREC_HORZ);
	}
	return SVG_STATUS_SUCCESS;
}

svg_status_t SvgGradient::AddStop(double offset, svg_color_t * color, double opacity)
{
	if(num_stops >= stops_size) {
		int old_size = stops_size;
		if(stops_size)
			stops_size *= 2;
		else
			stops_size = 2; /* Any useful gradient has at least 2 */
		SvgGradientStop * new_stops = (SvgGradientStop *)realloc(stops, stops_size*sizeof(SvgGradientStop));
		if(new_stops == NULL) {
			stops_size = old_size;
			return SVG_STATUS_NO_MEMORY;
		}
		stops = new_stops;
	}
	SvgGradientStop * stop = &stops[num_stops++];
	stop->offset = offset;
	stop->color = *color;
	stop->opacity = opacity;
	return SVG_STATUS_SUCCESS;
}

svg_status_t SvgGradient::ApplyAttributes(svg_t * svg, _XmlAttributes & rAttrList)
{
	SvgGradient * prototype = 0;
	SString href, attr;
	/* SPK: still an incomplete set of attributes */
	rAttrList.GetStr("xlink:href", 0, href);
	if(href.NotEmptyS()) {
		svg_element_t * ref = NULL;
		_svg_fetch_element_by_id(svg, href+1, &ref);
		if(ref && ref->type == SVG_ELEMENT_TYPE_GRADIENT) {
			SvgGradient save_gradient = *this;
			prototype = &ref->gradient;
			Init(prototype);
			if(type != save_gradient.type) {
				type = save_gradient.type;
				u = save_gradient.u;
			}
		}
	}
	if(rAttrList.GetStr("gradientUnits", "objectBoundingBox", attr) < 0 && prototype) {
		units = prototype->units;
	}
	else {
		if(attr.Cmp("userSpaceOnUse", 0) == 0)
			units = SVG_GRADIENT_UNITS_USER;
		else if(attr.Cmp("objectBoundingBox", 0) == 0)
			units = SVG_GRADIENT_UNITS_BBOX;
		else
			return SVG_STATUS_INVALID_VALUE;
	}
	rAttrList.GetStr("gradientTransform", 0, attr);
	if(attr.NotEmpty())
		transform.FromStr(attr, LMatrix2D::fmtSVG);
	else if(prototype)
		transform = prototype->transform;
	if(rAttrList.GetStr("spreadMethod", "pad", attr) < 0 && prototype)
		spread = prototype->spread;
	else {
		if(attr.Cmp("pad", 0) == 0)
			spread = SVG_GRADIENT_SPREAD_PAD;
		else if(attr.Cmp("reflect", 0) == 0)
			spread = SVG_GRADIENT_SPREAD_REFLECT;
		else if(attr.Cmp("repeat", 0) == 0)
			spread = SVG_GRADIENT_SPREAD_REPEAT;
		else
			return SVG_STATUS_INVALID_VALUE;
	}
	if(prototype && prototype->type != type)
		prototype = NULL;
	if(type == SVG_GRADIENT_LINEAR) {
		if(rAttrList.GetUSize("x1", "0%", u.linear.x1) < 0 && prototype)
			u.linear.x1 = prototype->u.linear.x1;
		if(rAttrList.GetUSize("y1", "0%", u.linear.y1) < 0 && prototype)
			u.linear.y1 = prototype->u.linear.y1;
		if(rAttrList.GetUSize("x2", "100%", u.linear.x2) < 0 && prototype)
			u.linear.x2 = prototype->u.linear.x2;
		if(rAttrList.GetUSize("y2", "0%", u.linear.y2) < 0 && prototype)
			u.linear.y2 = prototype->u.linear.y2;
	}
	else {
		if(rAttrList.GetUSize("cx", "50%", u.radial.cx) < 0 && prototype)
			u.radial.cx = prototype->u.radial.cx;
		if(rAttrList.GetUSize("cy", "50%", u.radial.cy) < 0 && prototype)
			u.radial.cy = prototype->u.radial.cy;
		if(rAttrList.GetUSize("r", "50%", u.radial.r) < 0 && prototype)
			u.radial.r = prototype->u.radial.r;
		/* fx and fy default to cx and cy */
		if(rAttrList.GetUSize("fx", "50%", u.radial.fx) < 0)
			u.radial.fx = u.radial.cx;
		if(rAttrList.GetUSize("fy", "50%", u.radial.fy) < 0)
			u.radial.fy = u.radial.cy;
	}
	return SVG_STATUS_SUCCESS;
}

//
// svg_group
//
static svg_status_t _svg_group_grow_element_by(svg_group_t * group, int additional);

svg_status_t _svg_group_init(svg_group_t * group)
{
	group->element = NULL;
	group->num_elements = 0;
	group->element_size = 0;
	group->width.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
	group->height.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
	group->view_box.AspectRatio = svg_view_box_t::pasUnkn;
	group->view_box.MeetOrSlice = svg_view_box_t::mosUnkn;
	group->x.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
	group->y.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_group_init_copy(svg_group_t * group, svg_group_t * other)
{
	svg_status_t status;
	svg_element_t * clone;
	int i;
	group->element = NULL;
	group->num_elements = 0;
	group->element_size = 0;
	/* clone children */
	for(i = 0; i < other->num_elements; i++) {
		status = (svg_status_t)_svg_element_clone(&clone, other->element[i]);
		if(status)
			return status;
		status = _svg_group_add_element(group, clone);
		if(status)
			return status;
	}
	group->width  = other->width;
	group->height = other->height;
	group->view_box = other->view_box;
	group->x = other->x;
	group->y = other->y;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_group_add_element(svg_group_t * group, svg_element_t * element)
{
	svg_status_t status;
	if(group->num_elements >= group->element_size) {
		int additional = group->element_size ? group->element_size : 4;
		status = _svg_group_grow_element_by(group, additional);
		if(status)
			return status;
	}
	group->element[group->num_elements] = element;
	group->num_elements++;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_group_render(svg_group_t * group, svg_render_engine_t * engine, void * closure)
{
	svg_status_t status, return_status = SVG_STATUS_SUCCESS;
	/* XXX: Perhaps this isn't the cleanest way to do this. It would
	   be cleaner to just immediately abort on an error I think. In
	   order to do that, we'd need to fix the parser so that it
	   doesn't include images with null data in the tree for
	   example. */
	for(int i = 0; i < group->num_elements; i++) {
		status = group->element[i]->render(engine, closure);
		if(status && !return_status)
			return_status = status;
	}
	return return_status;
}

svg_status_t _svg_symbol_render(svg_element_t * group, svg_render_engine_t * engine, void * closure)
{
	/* Never render a symbol directly. Only way to show a symbol is through <use>. */
	return SVG_STATUS_SUCCESS;
}
/* 
	Apply attributes unique to `svg' elements 
*/
svg_status_t _svg_group_apply_svg_attributes(svg_group_t * group, _XmlAttributes & rAttrList)
{
	svgint_status_t status;
	SString attr;
	rAttrList.GetUSize("width", "100%", group->width);
	rAttrList.GetUSize("height", "100%", group->height);
	/* XXX: What else? */
	rAttrList.GetUSize("x", "0", group->x);
	rAttrList.GetUSize("y", "0", group->y);
	rAttrList.GetStr("viewBox", 0, attr);
	if(attr.NotEmptyS()) {
		status = (svgint_status_t)group->view_box.ParseBox(attr);
		group->view_box.AspectRatio = svg_view_box_t::pasNone;
		rAttrList.GetStr("preserveAspectRatio", 0, attr);
		if(attr.NotEmptyS())
			group->view_box.ParseAspectRatio(attr);
	}
	return SVG_STATUS_SUCCESS;
}

/* Apply attributes common to `svg' and `g' elements */
svg_status_t _svg_group_apply_group_attributes(svg_group_t * group, const char ** attributes)
{
	/* XXX: NYI */
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_group_apply_use_attributes(svg_element_t * group, const char ** attributes)
{
	const char * href;
	svg_element_t * ref;
	svg_element_t * clone;
	svgint_status_t status;
	_svg_attribute_get_string(attributes, "xlink:href", &href, "");
	_svg_fetch_element_by_id(group->P_Doc, href+1, &ref);
	if(!ref) {
		/* XXX: Should we report an error here? */
		return SVG_STATUS_SUCCESS;
	}
	/*printf ("_svg_group_apply_use_attributes : %s\n", href + 1);
	   printf ("_svg_group_apply_use_attributes : %d\n", ref); */
	_svg_attribute_get_length(attributes, "width", &group->group.width, "100%");
	_svg_attribute_get_length(attributes, "height", &group->group.height, "100%");
	/* TODO : remove cloned tree (requires ref counting?). */
	status = _svg_element_clone(&clone, ref);
	if(status)
		return (svg_status_t)status;
	if(clone) {
		if(clone->type == SVG_ELEMENT_TYPE_SYMBOL) {
			clone->group.width = group->group.width;
			clone->group.height = group->group.height;
		}
		/* perform extra view_box transform for symbol */
		if(clone->type == SVG_ELEMENT_TYPE_SYMBOL &&
			clone->group.view_box.AspectRatio != svg_view_box_t::pasUnkn) {
			/*status = _svg_transform_apply_viewbox (&clone->transform, &clone->e.group.view_box,
			                                       clone->e.group.width, clone->e.group.height);*/
			clone->type = SVG_ELEMENT_TYPE_GROUP;
		}
		_svg_group_add_element(&group->group, clone);
	}
	_svg_attribute_get_length(attributes, "x", &group->group.x, "0");
	_svg_attribute_get_length(attributes, "y", &group->group.y, "0");
	/* _svg_transform_add_translate (&group->transform, _x, _y); */
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_group_grow_element_by(svg_group_t * group, int additional)
{
	svg_element_t ** new_element;
	int old_size = group->element_size;
	int new_size = group->num_elements+additional;
	if(new_size <= group->element_size) {
		return SVG_STATUS_SUCCESS;
	}
	group->element_size = new_size;
	new_element = (svg_element_t **)realloc(group->element, group->element_size*sizeof(svg_element_t *));
	if(new_element == NULL) {
		group->element_size = old_size;
		return SVG_STATUS_NO_MEMORY;
	}
	group->element = new_element;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_group_get_size(svg_group_t * group, USize * width, USize * height)
{
	*width = group->width;
	*height = group->height;
	return SVG_STATUS_SUCCESS;
}

//
// svg_image
//
static svg_status_t _svg_image_read_image(svg_image_t * image);
static svg_status_t _svg_image_read_png(const char * filename, char ** data, uint * width, uint * height);
static svg_status_t _svg_image_read_jpeg(const char * filename, char ** data, uint * width, uint * height);

svg_status_t _svg_image_init(svg_image_t * image)
{
	image->x.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
	image->y.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
	image->width.Set(0, UNIT_GR_PIXEL, DIREC_HORZ);
	image->height.Set(0, UNIT_GR_PIXEL, DIREC_VERT);
	image->url = NULL;
	image->data = NULL;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_image_init_copy(svg_image_t * image, svg_image_t * other)
{
	*image = *other;
	image->url = other->url ? strdup(other->url) : NULL;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_image_deinit(svg_image_t * image)
{
	ZFREE(image->url);
	ZFREE(image->data);
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_image_apply_attributes(svg_image_t * image, const char ** attributes)
{
	const char * aspect, * href;
	_svg_attribute_get_length(attributes, "x", &image->x, "0");
	_svg_attribute_get_length(attributes, "y", &image->y, "0");
	_svg_attribute_get_length(attributes, "width", &image->width, "0");
	_svg_attribute_get_length(attributes, "height", &image->height, "0");
	/* XXX: I'm not doing anything with preserveAspectRatio yet */
	_svg_attribute_get_string(attributes, "preserveAspectRatio", &aspect, "xMidyMid meet");
	/* XXX: This is 100% bogus with respect to the XML namespaces spec. */
	_svg_attribute_get_string(attributes, "xlink:href", &href, "");
	if(image->width < 0 || image->height < 0)
		return SVG_STATUS_PARSE_ERROR;
	/* XXX: We really need to do something like this to resolve
	   relative URLs. It involves linking the tree up in the other
	   direction. Or, another approach would be to simply throw out
	   the SAX parser and use the tree interface of libxml2 which
	   takes care of things like xml:base for us.

	   image->url = _svg_element_resolve_uri_alloc (image->element, href);

	   For now, the bogus code below will let me test the rest of the
	   image support:
	 */
	image->url = strdup((char *)href);
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_image_render(svg_image_t * image, svg_render_engine_t * engine, void * closure)
{
	svg_status_t status;
	if(image->width == 0.0 || image->height == 0.0)
		return SVG_STATUS_SUCCESS;
	status = _svg_image_read_image(image);
	if(status)
		return status;
	status = (engine->render_image)(closure, (uchar *)image->data,
		image->data_width, image->data_height, &image->x, &image->y, &image->width, &image->height);
	return status ? status : SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_image_read_image(svg_image_t * image)
{
	svgint_status_t status;
	if(image->data)
		return SVG_STATUS_SUCCESS;
	/* XXX: _svg_image_read_png only deals with filenames, not URLs */
	status = (svgint_status_t)_svg_image_read_png(image->url, &image->data, &image->data_width, &image->data_height);
	if(status == 0)
		return SVG_STATUS_SUCCESS;
	if(status != SVGINT_STATUS_IMAGE_NOT_PNG)
		return (svg_status_t)status;
	/* XXX: _svg_image_read_jpeg only deals with filenames, not URLs */
	status = (svgint_status_t)_svg_image_read_jpeg(image->url, &image->data, &image->data_width, &image->data_height);
	if(status == 0)
		return SVG_STATUS_SUCCESS;
	/* XXX: need to support SVG images as well */
	return (status != SVGINT_STATUS_IMAGE_NOT_JPEG) ? (svg_status_t)status : SVG_STATUS_PARSE_ERROR;
}

static void premultiply_data(png_structp png, png_row_infop row_info, png_bytep data)
{
	for(uint i = 0; i < row_info->rowbytes; i += 4) {
		uchar * b = &data[i];
		uchar alpha = b[3];
		unsigned long pixel = ((((b[0]*alpha)/255)<<0)|(((b[1]*alpha)/255)<<8)|(((b[2]*alpha)/255)<<16)|(alpha<<24));
		unsigned long * p = (unsigned long *)b;
		*p = pixel;
	}
}

static svg_status_t _svg_image_read_png(const char * filename, char ** data, uint * width, uint * height)
{
#define PNG_SIG_SIZE 8 // @sobolev {
// @sobolev static const int PNG_SIG_SIZE = 8;
	uint i;
	uchar png_sig[PNG_SIG_SIZE];
	int sig_bytes;
	png_struct * png;
	png_info * info;
	png_uint_32 png_width, png_height;
	int depth, color_type, interlace;
	uint pixel_size;
	png_byte ** row_pointers;
	FILE * file = fopen(filename, "rb");
	if(file == NULL)
		return SVG_STATUS_FILE_NOT_FOUND;
	sig_bytes = fread(png_sig, 1, PNG_SIG_SIZE, file);
	if(png_sig_cmp(png_sig, 0, sig_bytes) != 0) { // @sobolev
		// @sobolev if(png_check_sig(png_sig, sig_bytes) == 0) {
		fclose(file);
		return (svg_status_t)SVGINT_STATUS_IMAGE_NOT_PNG;
	}
	/* XXX: Perhaps we'll want some other error handlers? */
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png == NULL) {
		fclose(file);
		return SVG_STATUS_NO_MEMORY;
	}
	info = png_create_info_struct(png);
	if(info == NULL) {
		fclose(file);
		png_destroy_read_struct(&png, NULL, NULL);
		return SVG_STATUS_NO_MEMORY;
	}
	png_init_io(png, file);
	png_set_sig_bytes(png, sig_bytes);
	png_read_info(png, info);
	png_get_IHDR(png, info, &png_width, &png_height, &depth, &color_type, &interlace, NULL, NULL);
	*width = png_width;
	*height = png_height;
	/* XXX: I still don't know what formats will be exported in the
	   libsvg -> svg_render_engine interface. For now, I'm converting
	   everything to 32-bit RGBA. */
	/* convert palette/gray image to rgb */
	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	/* expand gray bit depth if needed */
	if(color_type == PNG_COLOR_TYPE_GRAY && depth < 8) {
		png_set_expand_gray_1_2_4_to_8(png); // @sobolev
		// @sobolev png_set_gray_1_2_4_to_8(png);
	}
	/* transform transparency to alpha */
	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);
	if(depth == 16)
		png_set_strip_16(png);
	if(depth < 8)
		png_set_packing(png);
	/* convert grayscale to RGB */
	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);
	if(interlace != PNG_INTERLACE_NONE)
		png_set_interlace_handling(png);
	png_set_bgr(png);
	png_set_filler(png, 0xff, PNG_FILLER_AFTER);
	png_set_read_user_transform_fn(png, premultiply_data);
	png_read_update_info(png, info);
	pixel_size = 4;
	*data = (char *)malloc(png_width*png_height*pixel_size);
	if(*data == NULL) {
		fclose(file);
		return SVG_STATUS_NO_MEMORY;
	}
	row_pointers = (png_byte **)malloc(png_height*sizeof(char *));
	for(i = 0; i < png_height; i++)
		row_pointers[i] = (png_byte *)(*data+i*png_width*pixel_size);
	png_read_image(png, row_pointers);
	png_read_end(png, info);
	free(row_pointers);
	fclose(file);
	png_destroy_read_struct(&png, &info, NULL);
	return SVG_STATUS_SUCCESS;
#undef PNG_SIG_SIZE // } @sobolev
}

typedef struct _svg_image_jpeg_err {
	struct jpeg_error_mgr pub; /* "public" fields */
	jmp_buf setjmp_buf;       /* for return to caller */
} svg_image_jpeg_err_t;

static void _svg_image_jpeg_error_exit(j_common_ptr cinfo)
{
	svgint_status_t status;
	svg_image_jpeg_err_t * err = (svg_image_jpeg_err_t *)cinfo->err;
	/* Are there any other error codes we might care about? */
	switch(err->pub.msg_code) {
	    case JERR_NO_SOI:
		status = SVGINT_STATUS_IMAGE_NOT_JPEG;
		break;
	    default:
		status = (svgint_status_t)SVG_STATUS_PARSE_ERROR;
		break;
	}
	longjmp(err->setjmp_buf, status);
}

static svg_status_t _svg_image_read_jpeg(const char * filename, char ** data, uint * width, uint * height)
{
	svgint_status_t status;
	struct jpeg_decompress_struct cinfo;
	svg_image_jpeg_err_t jpeg_err;
	JSAMPARRAY buf;
	int row_stride;
	uint i;
	uchar * out, * in;
	FILE * file = fopen(filename, "rb");
	if(file == NULL)
		return SVG_STATUS_FILE_NOT_FOUND;
	cinfo.err = jpeg_std_error(&jpeg_err.pub);
	jpeg_err.pub.error_exit = _svg_image_jpeg_error_exit;
	status = (svgint_status_t)setjmp(jpeg_err.setjmp_buf);
	if(status) {
		jpeg_destroy_decompress(&cinfo);
		fclose(file);
		return (svg_status_t)status;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	row_stride = cinfo.output_width*cinfo.output_components;
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	buf = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	*data = (char *)malloc(cinfo.output_width*cinfo.output_height*4);
	out = (uchar *)*data;
	while(cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, buf, 1);
		in = buf[0];
		for(i = 0; i < cinfo.output_width; i++) {
			switch(cinfo.num_components) {
			    case 1:
				out[3] = 0xff;
				out[2] = in[0];
				out[1] = in[1];
				out[0] = in[2];
				in += 1;
				out += 4;
				break;
			    default:
			    case 4:
				out[3] = 0xff;
				out[2] = in[0];
				out[1] = in[1];
				out[0] = in[2];
				in += 3;
				out += 4;
			}
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(file);
	return SVG_STATUS_SUCCESS;
}
//
// svg_paint
//
svg_status_t _svg_paint_init(svg_paint_t * paint, svg_t * svg, const char * str)
{
	svg_status_t status = SVG_STATUS_SUCCESS;
	if(strcmp(str, "none") == 0) {
		paint->type = SVG_PAINT_TYPE_NONE;
	}
	else if(strncmp(str, "url(#", 5) == 0 && strchr(str, ')')) { /* Paint parser courtesy of Steven Kramer */
		svg_element_t * element = NULL;
		const char * end = strchr(str, ')');
		int length = end-(str+5);
		char * id = (char *)malloc(length+1);
		if(!id)
			return SVG_STATUS_NO_MEMORY;
		else {
			strncpy(id, str+5, length);
			id[length] = '\0';
			_svg_fetch_element_by_id(svg, id, &element);
			free(id);
			if(element == NULL)
				status = SVG_STATUS_PARSE_ERROR;
			else {
				switch(element->type) {
					case SVG_ELEMENT_TYPE_GRADIENT:
						paint->type = SVG_PAINT_TYPE_GRADIENT;
						paint->p.gradient = &element->gradient;
						break;
					case SVG_ELEMENT_TYPE_PATTERN:
						paint->type = SVG_PAINT_TYPE_PATTERN;
						paint->p.pattern_element = element;
						break;
					default:
						status = SVG_STATUS_PARSE_ERROR;
				}
			}
		}
	}
	else {
		status = _svg_color_init_from_str(&paint->p.color, str);
		if(status == SVG_STATUS_SUCCESS)
			paint->type = SVG_PAINT_TYPE_COLOR;
	}
	return status;
}

svg_status_t _svg_paint_deinit(svg_paint_t * paint)
{
	switch(paint->type) {
	    case SVG_PAINT_TYPE_NONE: return SVG_STATUS_SUCCESS;
	    case SVG_PAINT_TYPE_GRADIENT:
	    case SVG_PAINT_TYPE_PATTERN:
		/* XXX: If we don't free the gradient/pattern paint here, do
		   we anywhere? Do we need to reference count them? */
		return SVG_STATUS_SUCCESS;
	}
	return SVG_STATUS_SUCCESS;
}
//
// svg_parser
//
static svg_status_t _svg_parser_parse_anchor(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_svg(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_defs(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_use(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_symbol(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_group(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element);
static svg_status_t _svg_parser_parse_path(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_line(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_rect(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_circle(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_ellipse(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_polygon(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_polyline(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_text(svg_parser_t * parser, const char ** attributes, svg_element_t ** text_element);
static svg_status_t _svg_parser_parse_image(svg_parser_t * parser, const char ** attributes, svg_element_t ** image_element);
static svg_status_t _svg_parser_parse_linear_gradient(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_radial_gradient(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_gradient_stop(svg_parser_t * parser, const char ** attributes, svg_element_t ** stop_element);
static svg_status_t _svg_parser_parse_pattern(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element);
static svg_status_t _svg_parser_parse_text_characters(svg_parser_t * parser, const char * ch, int len);

typedef struct svg_parser_map {
	char * name;
	svg_parser_cb_t cb;
} svg_parser_map_t;

static const svg_parser_map_t SVG_PARSER_MAP[] = {
	{"a",               {_svg_parser_parse_anchor,              NULL }},
	{"svg",             {_svg_parser_parse_svg,                 NULL }},
	{"g",               {_svg_parser_parse_group,               NULL }},
	{"path",            {_svg_parser_parse_path,                NULL }},
	{"line",            {_svg_parser_parse_line,                NULL }},
	{"rect",            {_svg_parser_parse_rect,                NULL }},
	{"circle",          {_svg_parser_parse_circle,              NULL }},
	{"ellipse",         {_svg_parser_parse_ellipse,             NULL }},
	{"defs",            {_svg_parser_parse_defs,                NULL }},
	{"use",             {_svg_parser_parse_use,                 NULL }},
	{"symbol",          {_svg_parser_parse_symbol,              NULL }},
	{"polygon",         {_svg_parser_parse_polygon,             NULL }},
	{"polyline",        {_svg_parser_parse_polyline,            NULL }},
	{"text",            {_svg_parser_parse_text,                _svg_parser_parse_text_characters }},
	{"image",           {_svg_parser_parse_image,               NULL }},
	{"linearGradient",  {_svg_parser_parse_linear_gradient,     NULL }},
	{"radialGradient",  {_svg_parser_parse_radial_gradient,     NULL }},
	{"stop",            {_svg_parser_parse_gradient_stop,       NULL }},
	{"pattern",         {_svg_parser_parse_pattern,             NULL }},
};

void _svg_parser_sax_start_element(void * closure, const xmlChar * name_unsigned, const xmlChar ** attributes_unsigned)
{
	uint i;
	svg_parser_t * parser = (svg_parser_t *)closure;
	const svg_parser_cb_t * cb;
	svg_element_t * element;
	const char * name = (const char *)name_unsigned;
	_XmlAttributes attr_list((const char **)attributes_unsigned);
	if(parser->unknown_element_depth) {
		parser->unknown_element_depth++;
		return;
	}
	cb = NULL;
#if 1 // {
	svg_status_t s = SVG_STATUS_SUCCESS;
	PushState(name);
	if(strcmp(name, "a") == 0) {
		s = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT;
	}
	else if(strcmp(name, "svg") == 0) {
		svg_element_t * parent = parser->state->group_element;
		s = (svg_status_t)_svg_element_create(&element, SVG_ELEMENT_TYPE_SVG_GROUP, parent, parser->svg);
		if(s == 0) {
			if(parent)
				s = _svg_group_add_element(&parent->group, element);
			else {
				_svg_style_init_defaults(&element->style, parser->svg);
				parser->svg->group_element = element;
			}
			parser->state->group_element = element;
		}
	}
	else if(strcmp(name, "g") == 0) {
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_GROUP);
	}
	else if(strcmp(name, "path") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_PATH);
	}
	else if(strcmp(name, "line") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_LINE);
		if(s == 0) {
			_svg_attribute_get_length(attr_list, "x1", &(element->e.line.x1), "0");
			_svg_attribute_get_length(attr_list, "y1", &(element->e.line.y1), "0");
			_svg_attribute_get_length(attr_list, "x2", &(element->e.line.x2), "0");
			_svg_attribute_get_length(attr_list, "y2", &(element->e.line.y2), "0");
		}
	}
	else if(strcmp(name, "rect") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_RECT);
		if(s)
			s = SVG_STATUS_PARSE_ERROR;
		else {
			int has_rx = 0, has_ry = 0;
			_svg_attribute_get_length(attr_list, "x", &(element->e.rect.x), "0");
			_svg_attribute_get_length(attr_list, "y", &(element->e.rect.y), "0");
			_svg_attribute_get_length(attr_list, "width", &(element->e.rect.width), "0");
			_svg_attribute_get_length(attr_list, "height", &(element->e.rect.height), "0");
			s = (svg_status_t)_svg_attribute_get_length(attr_list, "rx", &(element->e.rect.rx), "0");
			if(s == SVG_STATUS_SUCCESS)
				has_rx = 1;
			s = (svg_status_t)_svg_attribute_get_length(attr_list, "ry", &(element->e.rect.ry), "0");
			if(s == SVG_STATUS_SUCCESS)
				has_ry = 1;
			if(has_rx || has_ry) {
				if(!has_rx)
					element->e.rect.rx = element->e.rect.ry;
				if(!has_ry)
					element->e.rect.ry = element->e.rect.rx;
			}
		}
	}
	else if(strcmp(name, "circle") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_CIRCLE);
		if(s)
			s = SVG_STATUS_PARSE_ERROR;
		else {
			_svg_attribute_get_length(attr_list, "cx", &(element->e.ellipse.cx), "0");
			_svg_attribute_get_length(attr_list, "cy", &(element->e.ellipse.cy), "0");
			_svg_attribute_get_length(attr_list, "r",  &(element->e.ellipse.rx), "100%");
			_svg_attribute_get_length(attr_list, "r",  &(element->e.ellipse.ry), "100%");
			s = (element->e.ellipse.rx < 0.0) ? SVG_STATUS_PARSE_ERROR : SVG_STATUS_SUCCESS;
		}
	}
	else if(strcmp(name, "ellipse") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_ELLIPSE);
		if(s == 0) {
			_svg_attribute_get_length(attr_list, "cx", &(element->e.ellipse.cx), "0");
			_svg_attribute_get_length(attr_list, "cy", &(element->e.ellipse.cy), "0");
			_svg_attribute_get_length(attr_list, "rx", &(element->e.ellipse.rx), "100%");
			_svg_attribute_get_length(attr_list, "ry", &(element->e.ellipse.ry), "100%");
			if(element->e.ellipse.rx < 0.0 || element->e.ellipse.ry < 0.0)
				s = SVG_STATUS_PARSE_ERROR;
		}
	}
	else if(strcmp(name, "defs") == 0) {
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_DEFS);
	}
	else if(strcmp(name, "use") == 0) {
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_USE);
	}
	else if(strcmp(name, "symbol") == 0) {
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_SYMBOL);
	}
	else if(strcmp(name, "polygon") == 0) {
		s = _svg_parser_parse_polyline(parser, attr_list, &element);
		if(!s)
			s = _svg_path_close_path(&element->path);
	}
	else if(strcmp(name, "polyline") == 0) {
		const char * points;
		_svg_attribute_get_string(attr_list, "points", &points, NULL);
		if(points == NULL)
			s = SVG_STATUS_PARSE_ERROR;
		else {
			s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_PATH);
			if(s == 0) {
				svg_path_t * path = &element->path;
				const char * next;
				int first = 1;
				for(const char * p = points; *p;) {
					double pt[2];
					s = (svg_status_t)_svg_str_parse_csv_doubles(p, pt, 2, &next);
					if(s)
						break;
					else {
						if(first) {
							path->Move(FPoint((float)pt[0], (float)pt[1]));
							first = 0;
						}
						else {
							path->Line(FPoint((float)pt[0], (float)pt[1]));
						}
						p = next;
						_svg_str_skip_space(&p);
					}
				}
			}
		}
	}
	else if(strcmp(name, "text") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_TEXT);
		if(s == 0)
			parser->state->Txt = element->Txt;
		// uses _svg_parser_parse_text_characters
	}
	else if(strcmp(name, "image") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_IMAGE);
	}
	else if(strcmp(name, "linearGradient") == 0) {
		/* 
			XXX: This is really bogus. The gradient element is a subtype of
			svg_element_t that is distinct from the svg_group_t
			subtype. So, if any gradient happens to have a child element
			other than a gradient stop (which has hacked parsing to avoid
			the problem), then the svg_element union will be totally
			scrambled. This must be fixed. 
		*/
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_GRADIENT);
		if(s == 0)
			element->gradient.SetType(SVG_GRADIENT_LINEAR);
	}
	else if(strcmp(name, "radialGradient") == 0) {
		s = (svg_status_t)parser->NewGroupElement(&element, SVG_ELEMENT_TYPE_GRADIENT);
		if(s == 0)
			element->gradient.SetType(SVG_GRADIENT_RADIAL);
	}
	else if(strcmp(name, "stop") == 0) {
		/* 
			XXX: This function is a mess --- it's doing its own style/attribute
			handling rather than leaving that to the existing
			framework. Instead this should be shifted to the standard
			mechanism, whereby we have a new svg_gradient_stop element, etc.

			If we'd like to, we can collapse the gradient's child stop elements
			into an array when the gradient is done being parsed.  
		*/
		//static svg_status_t _svg_parser_parse_gradient_stop(svg_parser_t * parser, const char ** attributes, svg_element_t ** gradient_element)
		{
			svg_style_t style;
			SvgGradient * gradient;
			double offset;
			double opacity;
			svg_color_t color;
			const char * color_str;
			svg_element_t * group_element;
			svg_element_t ** gradient_element = NULL;
			//
			if(parser->state->group_element == NULL || parser->state->group_element->type != SVG_ELEMENT_TYPE_GRADIENT)
				s = SVG_STATUS_PARSE_ERROR;
			else {
				group_element = parser->state->group_element;
				gradient = &group_element->gradient;
				/* XXX: This ad-hoc style parsing breaks inheritance I believe. */
				style.Init(parser->svg);
				style.flags = SVG_STYLE_FLAG_NONE;
				_svg_style_apply_attributes(&style, attr_list);
				color = style.color;
				opacity = style.opacity;
				_svg_attribute_get_double(attr_list, "offset", &offset, 0);
				_svg_attribute_get_double(attr_list, "stop-opacity", &opacity, opacity);
				if(_svg_attribute_get_string(attr_list, "stop-color", &color_str, "#000000") == SVG_STATUS_SUCCESS)
					_svg_color_init_from_str(&color, color_str);
				if(color.is_current_color)
					color = group_element->style.color;
				/* XXX: Rather than directly storing the stop in the gradient
				here, it would be cleaner to just have the stop be a standard
				child element. */
				gradient->AddStop(offset, &color, opacity);
				/* XXX: Obviously, this is totally bogus and needs to change. */
				/* not quite unknown, just don't store the element and stop applying attributes */
				//s = (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT;
				s = SVG_STATUS_SUCCESS;
			}
		}
	}
	else if(strcmp(name, "pattern") == 0) {
		s = (svg_status_t)parser->NewLeafElement(&element, SVG_ELEMENT_TYPE_PATTERN);
		if(s == 0) {
			/*
				XXX: This is a bit messy. The pattern has child elements, but
				is not of SVG_ELEMENT_TYPE_GROUP, but rather contains one. We
				need to set that containend group as the parent for future components.
			*/
			parser->state->group_element = element->pattern.group_element;
		}
	}
	else {
		parser->unknown_element_depth++;
		PopState();
	}
	parser->status = s;
#endif // } 0
	for(i = 0; i < SIZEOFARRAY(SVG_PARSER_MAP); i++) {
		if(strcmp(SVG_PARSER_MAP[i].name, name) == 0) {
			cb = &SVG_PARSER_MAP[i].cb;
			break;
		}
	}
	if(cb == NULL) {
		parser->unknown_element_depth++;
		return;
	}
	else {
		parser->PushState();
		parser->status = (cb->parse_element)(parser, attr_list, &element);
		if(parser->status) {
			if(parser->status == SVGINT_STATUS_UNKNOWN_ELEMENT)
				parser->status = SVG_STATUS_SUCCESS;
			return;
		}
		parser->status = _svg_element_apply_attributes(element, attr_list);
		if(parser->status == 0 && element->Ident.NotEmpty())
			_svg_xml_hash_add_entry((svg_xml_hash_table_t *)parser->svg->element_ids, (const xmlChar *)(const char *)element->Ident, element);
	}
}

void _svg_parser_sax_end_element(void * closure, const xmlChar * name)
{
	svg_parser_t * parser = (svg_parser_t *)closure;
	if(parser->unknown_element_depth)
		parser->unknown_element_depth--;
	else
		parser->PopState();
}

void _svg_parser_sax_characters(void * closure, const xmlChar * ch_unsigned, int len)
{
	int i;
	svg_parser_t * parser = (svg_parser_t *)closure;
	const char * src, * ch = (const char *)ch_unsigned;
	char  * dst;
	int space;

	/* XXX: This is the correct default behavior, but we're supposed
	 * to honor xml:space="preserve" if present, (which just means to
	 * not do this replacement).
	 */
	char * ch_copy = (char *)malloc(len);
	if(ch_copy == NULL)
		return;
	dst = ch_copy;
	space = 0;
	for(src = ch, i = 0; i < len; i++, src++) {
		if(*src == '\n')
			continue;
		if(*src == '\t' || *src == ' ') {
			if(space)
				continue;
			*dst = ' ';
			space = 1;
		}
		else {
			*dst = *src;
			space = 0;
		}
		dst++;
	}
	if(parser->state->cb->parse_characters) {
		parser->status = (parser->state->cb->parse_characters)(parser, ch_copy, dst-ch_copy);
		if(parser->status)
			return;
	}
	ZFREE(ch_copy);
}

int svg_parser_t::PushState(const char * pTag)
{
	int    ok = 1;
	State * p_state = new State;
	if(p_state) {
		p_state->Tag = pTag;
		if(state) {
			*p_state = *state;
		}
		else {
			p_state->group_element = NULL;
			p_state->Txt = 0;
		}
		p_state->cb = cb;
		p_state->next = state;
		state = p_state;
		status = SVG_STATUS_SUCCESS;
	}
	else {
		parser->status = SVG_STATUS_NO_MEMORY;
		ok = 0;
	}
	return ok;
}

int svg_parser_t::PopState()
{
	int    ok = 1;
	if(state) {
		State * p_old = state;
		state = state->next;
		delete p_old;
	}
	else
		ok = -1;
	status = SVG_STATUS_SUCCESS;
	return ok;
}

int svg_parser_t::NewLeafElement(svg_element_t ** child_element, svg_element_type_t type)
{
	svg_status_t s = (svg_status_t)_svg_element_create(child_element, type, state->group_element, svg);
	if(s == 0)
		s = _svg_group_add_element(&state->group_element->group, *child_element);
	return s ? s : SVG_STATUS_SUCCESS;
}

int svg_parser_t::NewGroupElement(svg_element_t ** group_element, svg_element_type_t type)
{
	svg_status_t s = (svg_status_t)NewLeafElement(group_element, type);
	if(s == 0)
		//
		// The only thing that distinguishes a group from a leaf is that
		// the group becomes the new parent for future elements.
		//
		state->group_element = *group_element;
	return s;
}

static svg_status_t _svg_parser_parse_anchor(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	/* XXX: Currently ignoring all anchor elements */
	return (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT;
}

static svg_status_t _svg_parser_parse_svg(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	svg_element_t * parent = parser->state->group_element;
	svg_status_t status = (svg_status_t)_svg_element_create(group_element, SVG_ELEMENT_TYPE_SVG_GROUP, parent, parser->svg);
	if(status)
		return status;
	if(parent) {
		status = _svg_group_add_element(&parent->group, *group_element);
	}
	else {
		_svg_style_init_defaults(&(*group_element)->style, parser->svg);
		parser->svg->group_element = *group_element;
	}
	parser->state->group_element = *group_element;
	return status;
}

static svg_status_t _svg_parser_parse_group(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	return (svg_status_t)parser->NewGroupElement(group_element, SVG_ELEMENT_TYPE_GROUP);
}

static svg_status_t _svg_parser_parse_defs(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	return (svg_status_t)parser->NewGroupElement(group_element, SVG_ELEMENT_TYPE_DEFS);
}

static svg_status_t _svg_parser_parse_use(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	return (svg_status_t)parser->NewGroupElement(group_element, SVG_ELEMENT_TYPE_USE);
}

static svg_status_t _svg_parser_parse_symbol(svg_parser_t * parser, const char ** attributes, svg_element_t ** group_element)
{
	return (svg_status_t)parser->NewGroupElement(group_element, SVG_ELEMENT_TYPE_SYMBOL);
}

static svg_status_t _svg_parser_parse_path(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	return (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_PATH);
}

static svg_status_t _svg_parser_parse_line(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	svg_status_t status = (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_LINE);
	if(status)
		return status;
	_svg_attribute_get_length(attributes, "x1", &((*path_element)->e.line.x1), "0");
	_svg_attribute_get_length(attributes, "y1", &((*path_element)->e.line.y1), "0");
	_svg_attribute_get_length(attributes, "x2", &((*path_element)->e.line.x2), "0");
	_svg_attribute_get_length(attributes, "y2", &((*path_element)->e.line.y2), "0");
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_rect(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	int has_rx = 0, has_ry = 0;
	svg_status_t status = (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_RECT);
	if(status)
		return SVG_STATUS_PARSE_ERROR;
	_svg_attribute_get_length(attributes, "x", &((*path_element)->e.rect.x), "0");
	_svg_attribute_get_length(attributes, "y", &((*path_element)->e.rect.y), "0");
	_svg_attribute_get_length(attributes, "width", &((*path_element)->e.rect.width), "0");
	_svg_attribute_get_length(attributes, "height", &((*path_element)->e.rect.height), "0");
	status = (svg_status_t)_svg_attribute_get_length(attributes, "rx", &((*path_element)->e.rect.rx), "0");
	if(status == SVG_STATUS_SUCCESS)
		has_rx = 1;
	status = (svg_status_t)_svg_attribute_get_length(attributes, "ry", &((*path_element)->e.rect.ry), "0");
	if(status == SVG_STATUS_SUCCESS)
		has_ry = 1;
	if(has_rx || has_ry) {
		if(!has_rx)
			(*path_element)->e.rect.rx = (*path_element)->e.rect.ry;
		if(!has_ry)
			(*path_element)->e.rect.ry = (*path_element)->e.rect.rx;
	}
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_circle(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	svg_status_t status = (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_CIRCLE);
	if(status)
		return SVG_STATUS_PARSE_ERROR;
	_svg_attribute_get_length(attributes, "cx", &((*path_element)->e.ellipse.cx), "0");
	_svg_attribute_get_length(attributes, "cy", &((*path_element)->e.ellipse.cy), "0");
	_svg_attribute_get_length(attributes, "r", &((*path_element)->e.ellipse.rx), "100%");
	_svg_attribute_get_length(attributes, "r", &((*path_element)->e.ellipse.ry), "100%");
	return ((*path_element)->e.ellipse.rx < 0.0) ? SVG_STATUS_PARSE_ERROR : SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_ellipse(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	svg_status_t status = (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_ELLIPSE);
	if(status)
		return status;
	_svg_attribute_get_length(attributes, "cx", &((*path_element)->e.ellipse.cx), "0");
	_svg_attribute_get_length(attributes, "cy", &((*path_element)->e.ellipse.cy), "0");
	_svg_attribute_get_length(attributes, "rx", &((*path_element)->e.ellipse.rx), "100%");
	_svg_attribute_get_length(attributes, "ry", &((*path_element)->e.ellipse.ry), "100%");
	if((*path_element)->e.ellipse.rx < 0.0 || (*path_element)->e.ellipse.ry < 0.0)
		return SVG_STATUS_PARSE_ERROR;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_polygon(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	svg_status_t status = _svg_parser_parse_polyline(parser, attributes, path_element);
	if(!status) {
		svg_path_t * path = &(*path_element)->path;
		status = _svg_path_close_path(path);
	}
	return status ? status : SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_polyline(svg_parser_t * parser, const char ** attributes, svg_element_t ** path_element)
{
	svg_status_t status;
	const char * points;
	const char * p, * next;
	svg_path_t * path;
	double pt[2];
	int first;
	_svg_attribute_get_string(attributes, "points", &points, NULL);
	if(points == NULL)
		return SVG_STATUS_PARSE_ERROR;
	status = (svg_status_t)parser->NewLeafElement(path_element, SVG_ELEMENT_TYPE_PATH);
	if(status)
		return status;
	path = &(*path_element)->path;
	first = 1;
	p = points;
	while(*p) {
		status = (svg_status_t)_svg_str_parse_csv_doubles(p, pt, 2, &next);
		if(status)
			return SVG_STATUS_PARSE_ERROR;
		if(first) {
			path->Move(FPoint((float)pt[0], (float)pt[1]));
			first = 0;
		}
		else {
			path->Line(FPoint((float)pt[0], (float)pt[1]));
		}
		p = next;
		_svg_str_skip_space(&p);
	}
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_text(svg_parser_t * parser, const char ** attributes, svg_element_t ** text_element)
{
	svg_status_t status = (svg_status_t)parser->NewLeafElement(text_element, SVG_ELEMENT_TYPE_TEXT);
	if(status == 0)
		parser->state->Txt = (*text_element)->Txt;
	return status;
}

static svg_status_t _svg_parser_parse_image(svg_parser_t * parser, const char ** attributes, svg_element_t ** image_element)
{
	return (svg_status_t)parser->NewLeafElement(image_element, SVG_ELEMENT_TYPE_IMAGE);
}

/* Gradient parsing code by Steven Kramer */

static svg_status_t _svg_parser_parse_linear_gradient(svg_parser_t * parser, const char ** attributes, svg_element_t ** gradient_element)
{
	/* XXX: This is really bogus. The gradient element is a subtype of
	   svg_element_t that is distinct from the svg_group_t
	   subtype. So, if any gradient happens to have a child element
	   other than a gradient stop (which has hacked parsing to avoid
	   the problem), then the svg_element union will be totally
	   scrambled. This must be fixed. */
	svg_status_t status = (svg_status_t)parser->NewGroupElement(gradient_element, SVG_ELEMENT_TYPE_GRADIENT);
	if(status)
		return status;
	(*gradient_element)->gradient.SetType(SVG_GRADIENT_LINEAR);
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_radial_gradient(svg_parser_t * parser, const char ** attributes, svg_element_t ** gradient_element)
{
	svg_status_t status = (svg_status_t)parser->NewGroupElement(gradient_element, SVG_ELEMENT_TYPE_GRADIENT);
	if(status)
		return status;
	(*gradient_element)->gradient.SetType(SVG_GRADIENT_RADIAL);
	return SVG_STATUS_SUCCESS;
}

/* XXX: This function is a mess --- it's doing its own style/attribute
   handling rather than leaving that to the existing
   framework. Instead this should be shifted to the standard
   mechanism, whereby we have a new svg_gradient_stop element, etc.

   If we'd like to, we can collapse the gradient's child stop elements
   into an array when the gradient is done being parsed.  */
static svg_status_t _svg_parser_parse_gradient_stop(svg_parser_t * parser, const char ** attributes, svg_element_t ** gradient_element)
{
	svg_style_t style;
	SvgGradient * gradient;
	double offset;
	double opacity;
	svg_color_t color;
	const char * color_str;
	svg_element_t * group_element;
	gradient_element = NULL;
	if(parser->state->group_element == NULL || parser->state->group_element->type != SVG_ELEMENT_TYPE_GRADIENT)
		return SVG_STATUS_PARSE_ERROR;
	group_element = parser->state->group_element;
	gradient = &group_element->gradient;
	/* XXX: This ad-hoc style parsing breaks inheritance I believe. */
	style.Init(parser->svg);
	style.flags = SVG_STYLE_FLAG_NONE;
	_svg_style_apply_attributes(&style, attributes);
	color = style.color;
	opacity = style.opacity;
	_svg_attribute_get_double(attributes, "offset", &offset, 0);
	_svg_attribute_get_double(attributes, "stop-opacity", &opacity, opacity);
	if(_svg_attribute_get_string(attributes, "stop-color", &color_str, "#000000") == SVG_STATUS_SUCCESS)
		_svg_color_init_from_str(&color, color_str);
	if(color.is_current_color)
		color = group_element->style.color;
	/* XXX: Rather than directly storing the stop in the gradient
	   here, it would be cleaner to just have the stop be a standard
	   child element. */
	gradient->AddStop(offset, &color, opacity);
	/* XXX: Obviously, this is totally bogus and needs to change. */
	/* not quite unknown, just don't store the element and stop applying attributes */
	return (svg_status_t)SVGINT_STATUS_UNKNOWN_ELEMENT;
}

static svg_status_t _svg_parser_parse_pattern(svg_parser_t * parser, const char ** attributes, svg_element_t ** pattern_element)
{
	svg_status_t status = (svg_status_t)parser->NewLeafElement(pattern_element, SVG_ELEMENT_TYPE_PATTERN);
	/*
		XXX: This is a bit messy. The pattern has child elements, but
		is not of SVG_ELEMENT_TYPE_GROUP, but rather contains one. We
		need to set that containend group as the parent for future components.
	*/
	parser->state->group_element = (*pattern_element)->pattern.group_element;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_parser_parse_text_characters(svg_parser_t * parser, const char * chars, int len)
{
	parser->state->Txt.CatN(chars, len);
	return SVG_STATUS_SUCCESS;
}

//
// svg_parser_libxml
//
static xmlEntity * _svg_parser_sax_get_entity(void * closure, const xmlChar * name);
static void _svg_parser_sax_entity_decl(void * closure, const xmlChar * name, int type, const xmlChar * publicId,
	const xmlChar * systemId, xmlChar * content);
static void _svg_parser_sax_warning(void * closure, const char * msg, ...);
static void _svg_parser_sax_error(void * closure, const char * msg, ...);
static void _svg_parser_sax_fatal_error(void * closure, const char * msg, ...);

static xmlSAXHandler SVG_PARSER_SAX_HANDLER = {
	NULL,                           /* internalSubset */
	NULL,                           /* isStandalone */
	NULL,                           /* hasInternalSubset */
	NULL,                           /* hasExternalSubset */
	NULL,                           /* resolveEntity */
	_svg_parser_sax_get_entity,     /* getEntity */
	_svg_parser_sax_entity_decl,    /* entityDecl */
	NULL,                           /* notationDecl */
	NULL,                           /* attributeDecl */
	NULL,                           /* elementDecl */
	NULL,                           /* unparsedEntityDecl */
	NULL,                           /* setDocumentLocator */
	NULL,                           /* startDocument */
	NULL,                           /* endDocument */
	_svg_parser_sax_start_element,  /* startElement */
	_svg_parser_sax_end_element,    /* endElement */
	NULL,                           /* reference */
	_svg_parser_sax_characters,     /* characters */
	_svg_parser_sax_characters,     /* ignorableWhitespace */
	NULL,                           /* processingInstruction */
	NULL,                           /* comment */
	_svg_parser_sax_warning,        /* xmlParserWarning */
	_svg_parser_sax_error,          /* xmlParserError */
	_svg_parser_sax_fatal_error,    /* xmlParserFatalError */
	NULL,                           /* getParameterEntity */
};

svg_status_t svg_parse_chunk_begin(svg_t * svg)
{
	/* Innocent until proven guilty */
	svg->parser.status = SVG_STATUS_SUCCESS;
	if(svg->parser.ctxt)
		svg->parser.status = SVG_STATUS_INVALID_CALL;
	svg->parser.ctxt = xmlCreatePushParserCtxt(&SVG_PARSER_SAX_HANDLER, &svg->parser, NULL, 0, NULL);
	if(svg->parser.ctxt == NULL)
		svg->parser.status = SVG_STATUS_NO_MEMORY;
	((svg_xml_parser_context_t)svg->parser.ctxt)->replaceEntities = 1;
	svg->parser.entities = xmlHashCreate(100);
	return svg->parser.status;
}

static xmlEntity * _svg_parser_sax_get_entity(void * closure, const xmlChar * name)
{
	svg_parser_t * parser = (svg_parser_t *)closure;
	return (xmlEntity *)xmlHashLookup((svg_xml_hash_table_t *)parser->entities, name);
}

/* XXX: It's not clear to my why we have to do this. libxml2 has all
 * of this code internally, so we should be able to access a hash of
 * entities automatically rather than maintaining our own. */
static void _svg_parser_sax_entity_decl(void * closure, const xmlChar * name, int type, const xmlChar * publicId,
	const xmlChar * systemId, xmlChar * content)
{
	svg_parser_t * parser = (svg_parser_t *)closure;
	xmlEntityPtr entity = (xmlEntityPtr)malloc(sizeof(xmlEntity));
	entity->type = XML_ENTITY_DECL;
	entity->name = xmlStrdup(name);
	entity->etype = (xmlEntityType)type;
	entity->ExternalID = publicId ? xmlStrdup(publicId) : NULL;
	entity->SystemID = systemId ? xmlStrdup(systemId) : NULL;
	if(content) {
		entity->length = xmlStrlen(content);
		entity->content = xmlStrndup(content, entity->length);
	}
	else {
		entity->length = 0;
		entity->content = NULL;
	}
	entity->URI = NULL;
	entity->orig = NULL;
	entity->owner = 0;
	entity->children = NULL;
	if(xmlHashAddEntry((svg_xml_hash_table_t *)parser->entities, name, entity))
		free(entity); /* Entity was already defined at another level. */
}

static void _svg_parser_sax_warning(void * closure, const char * msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

static void _svg_parser_sax_error(void * closure, const char * msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

static void _svg_parser_sax_fatal_error(void * closure, const char * msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}
//
// svg_path
//
svg_status_t svg_path_t::Render(svg_render_engine_t * engine, void * closure)
{
	svg_status_t status = (svg_status_t)0;
	for(uint i = 0; i < GetCount(); i++) {
		const SDrawPath::Item * p_item = Get(i);
		if(p_item) {
			const float * p_arg = p_item->ArgList;
			switch(p_item->Op) {
				case SDrawPath::opMove:
					status = (engine->move_to)(closure, p_arg[0], p_arg[1]);
					break;
				case SDrawPath::opLine:
					status = (engine->line_to)(closure, p_arg[0], p_arg[1]);
					break;
				case SDrawPath::opCurve:
					status = (engine->curve_to)(closure, p_arg[0], p_arg[1], p_arg[2], p_arg[3], p_arg[4], p_arg[5]);
					break;
				case SDrawPath::opQuad:
					status = (engine->quadratic_curve_to)(closure, p_arg[0], p_arg[1], p_arg[2], p_arg[3]);
					break;
				case SDrawPath::opArcSvg:
					status = (engine->arc_to)(closure, p_arg[0], p_arg[1], p_arg[2], (int)p_arg[3], (int)p_arg[4], p_arg[5], p_arg[6]);
					break;
				case SDrawPath::opClose:
					status = (engine->close_path)(closure);
					break;
			}
		}
	}
	status = (engine->render_path)(closure);
	return status ? status : SVG_STATUS_SUCCESS;
}

svg_status_t _svg_path_close_path(svg_path_t * path)
{
	path->Close();
	return SVG_STATUS_SUCCESS;
}
//
// svg_pattern
//
svg_status_t _svg_pattern_init(svg_pattern_t * pattern, svg_element_t * parent, svg_t * doc)
{
	svg_status_t status;
	pattern->x.Set(0, UNIT_PERCENT, DIREC_HORZ);
	pattern->y.Set(0, UNIT_PERCENT, DIREC_HORZ);
	pattern->width.Set(0, UNIT_PERCENT, DIREC_HORZ);
	pattern->height.Set(0, UNIT_PERCENT, DIREC_HORZ);
	status = (svg_status_t)_svg_element_create(&pattern->group_element, SVG_ELEMENT_TYPE_GROUP, parent, doc);
	return status ? status : SVG_STATUS_SUCCESS;
}

svg_status_t _svg_pattern_init_copy(svg_pattern_t * pattern, svg_pattern_t * other)
{
	svg_status_t status = (svg_status_t)_svg_element_clone(&pattern->group_element, other->group_element);
	if(status)
		return status;
	pattern->units = other->units;
	pattern->content_units = other->content_units;
	pattern->x = other->x;
	pattern->y = other->y;
	pattern->width = other->width;
	pattern->height = other->height;
	pattern->transform = other->transform;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_pattern_deinit(svg_pattern_t * pattern)
{
	_svg_element_destroy(pattern->group_element);
	pattern->group_element = NULL;
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_pattern_apply_attributes(svg_pattern_t * pattern, const char ** attributes)
{
	LMatrix2D transform;
	char const * str;
	_svg_attribute_get_string(attributes, "patternUnits", &str, "objectBoundingBox");
	if(strcmp(str, "userSpaceOnUse") == 0) {
		pattern->units = SVG_PATTERN_UNITS_USER;
	}
	else if(strcmp(str, "objectBoundingBox") == 0) {
		pattern->units = SVG_PATTERN_UNITS_BBOX;
	}
	else {
		return SVG_STATUS_INVALID_VALUE;
	}
	_svg_attribute_get_string(attributes, "patternContentUnits", &str, "userSpaceOnUse");
	if(strcmp(str, "userSpaceOnUse") == 0) {
		pattern->content_units = SVG_PATTERN_UNITS_USER;
	}
	else if(strcmp(str, "objectBoundingBox") == 0) {
		pattern->content_units = SVG_PATTERN_UNITS_BBOX;
	}
	else {
		return SVG_STATUS_INVALID_VALUE;
	}
	_svg_attribute_get_length(attributes, "x", &pattern->x, "0");
	_svg_attribute_get_length(attributes, "y", &pattern->y, "0");
	_svg_attribute_get_length(attributes, "width", &pattern->width, "0");
	_svg_attribute_get_length(attributes, "height", &pattern->height, "0");
	transform.InitUnit();
	_svg_attribute_get_string(attributes, "patternTransform", &str, 0);
	if(str)
		transform.FromStr(str, LMatrix2D::fmtSVG);
	pattern->transform = transform;
	return SVG_STATUS_SUCCESS;
}

//
// svg_str
//
void _svg_str_skip_space(const char ** str)
{
	const char * s = *str;
	while(_svg_ascii_isspace(*s))
		s++;
	*str = s;
}

void _svg_str_skip_char(const char ** str, char c)
{
	const char * s = *str;
	while(*s == c)
		s++;
	*str = s;
}

void _svg_str_skip_space_or_char(const char ** str, char c)
{
	const char * s = *str;
	while(_svg_ascii_isspace(*s) || *s == c)
		s++;
	*str = s;
}

svgint_status_t _svg_str_parse_csv_doubles(const char * str, double * value, int num_values, const char ** end)
{
	int i;
	double val;
	const char * fail_pos = str;
	svg_status_t status = SVG_STATUS_SUCCESS;
	for(i = 0; i < num_values; i++) {
		_svg_str_skip_space_or_char(&str, ',');
		if(*str == '\0') {
			fail_pos = str;
			status = (svg_status_t)SVGINT_STATUS_ARGS_EXHAUSTED;
			break;
		}
		val = _svg_ascii_strtod(str, &fail_pos);
		if(fail_pos == str) {
			status = (svg_status_t)SVGINT_STATUS_ARGS_EXHAUSTED;
			break;
		}
		str = fail_pos;
		value[i] = val;
	}
	ASSIGN_PTR(end, fail_pos);
	return (svgint_status_t)status;
}

svgint_status_t _svg_str_parse_all_csv_doubles(const char * str, double ** value, int * num_values, const char ** end)
{
	svgint_status_t status;
	int size = 0;
	*num_values = 0;
	*value = NULL;
	while(1) {
		if(*num_values >= size) {
			while(*num_values >= size)
				size = size ? size*2 : 5;
			*value = (double *)realloc(*value, size*sizeof(double));
			if(*value == NULL)
				status = (svgint_status_t)SVG_STATUS_NO_MEMORY;
		}
		status = _svg_str_parse_csv_doubles(str, *value+*num_values, 1, end);
		if(status)
			break;
		(*num_values)++;
		str = *end;
	}
	if(status == SVGINT_STATUS_ARGS_EXHAUSTED)
		status = (svgint_status_t)SVG_STATUS_SUCCESS;
	return status;
}

//
// svg_style
//
static svg_status_t _svg_style_parse_color(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_display(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_fill_opacity(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_fill_paint(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_fill_rule(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_font_family(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_font_size(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_font_style(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_font_weight(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_opacity(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_dash_array(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_dash_offset(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_line_cap(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_line_join(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_miter_limit(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_opacity(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_paint(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stroke_width(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_text_anchor(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_visibility(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stop_color(svg_style_t * style, const char * str);
static svg_status_t _svg_style_parse_stop_opacity(svg_style_t * style, const char * str);
static svg_status_t _svg_style_str_to_opacity(const char * str, double * ret);
static svg_status_t _svg_style_parse_nv_pair(svg_style_t * style, const char * nv_pair);
static svg_status_t _svg_style_parse_style_str(svg_style_t * style, const char * str);

typedef struct svg_style_parse_map {
	const char * name;
	svg_status_t (* parse)(svg_style_t * style, const char * value);
	const char * default_value;
} svg_style_parse_map_t;

static const svg_style_parse_map_t SVG_STYLE_PARSE_MAP[] = {
/* XXX: { "clip-rule",		_svg_style_parse_clip_rule,		"nonzero" }, */
	{ "color",                  _svg_style_parse_color,                 "black" },
/* XXX: { "color-interpolation",_svg_style_parse_color_interpolation,	"sRGB" }, */
/* XXX: { "color-interpolation-filters",_svg_style_parse_color_interpolation_filters,	"linearRGB" }, */
/* XXX: { "color-profile",	_svg_style_parse_color_profile,		"auto" }, */
/* XXX: { "color-rendering",	_svg_style_parse_color_rendering,	"auto" }, */
/* XXX: { "cursor",		_svg_style_parse_cursor,		"auto" }, */
/* XXX: { "direction",		_svg_style_parse_direction,		"ltr" }, */
	{ "display",                _svg_style_parse_display,               "inline" },
	{ "fill-opacity",           _svg_style_parse_fill_opacity,          "1.0" },
	{ "fill",                   _svg_style_parse_fill_paint,            "black" },
	{ "fill-rule",              _svg_style_parse_fill_rule,             "nonzero" },
/* XXX: { "font",		_svg_style_parse_font,			NULL }, */
	{ "font-family",            _svg_style_parse_font_family,           "sans-serif" },
	/* XXX: The default is supposed to be "medium" but I'm not parsing that yet */
	{ "font-size",              _svg_style_parse_font_size,             "10.0" },
/* XXX: { "font-size-adjust",	_svg_style_parse_font_size_adjust,	"none" }, */
/* XXX: { "font-stretch",	_svg_style_parse_font_stretch,		"normal" }, */
	{ "font-style",             _svg_style_parse_font_style,            "normal" },
/* XXX: { "font-variant",	_svg_style_parse_font_variant,		"normal" }, */
	{ "font-weight",            _svg_style_parse_font_weight,           "normal" },
/* XXX: { "glyph-orientation-horizontal",	_svg_style_parse_glyph_orientation_horizontal,	"0deg" }, */
/* XXX: { "glyph-orientation-vertical",		_svg_style_parse_glyph_orientation_vertical,	"auto" }, */
/* XXX: { "image-rendering",	_svg_style_parse_image_rendering,	"auto" }, */
/* XXX: { "kerning",		_svg_style_parse_kerning,		"auto" }, */
/* XXX: { "letter-spacing",	_svg_style_parse_letter_spacing,	"normal" }, */
/* XXX: { "marker",		_svg_style_parse_marker,		NULL }, */
/* XXX: { "marker-end",		_svg_style_parse_marker_end,		"none" }, */
/* XXX: { "marker-mid",		_svg_style_parse_marker_mid,		"none" }, */
/* XXX: { "marker-start",	_svg_style_parse_marker_start,		"none" }, */
	{ "opacity",                _svg_style_parse_opacity,               "1.0" },
/* XXX: { "pointer-events",	_svg_style_parse_pointer_events,	"visiblePainted" }, */
/* XXX: { "shape-rendering",	_svg_style_parse_shape_rendering,	"auto" }, */
	{ "stroke-dasharray",       _svg_style_parse_stroke_dash_array,     "none" },
	{ "stroke-dashoffset",      _svg_style_parse_stroke_dash_offset,    "0.0" },
	{ "stroke-linecap",         _svg_style_parse_stroke_line_cap,       "butt" },
	{ "stroke-linejoin",        _svg_style_parse_stroke_line_join,      "miter" },
	{ "stroke-miterlimit",      _svg_style_parse_stroke_miter_limit,    "4.0" },
	{ "stroke-opacity",         _svg_style_parse_stroke_opacity,        "1.0" },
	{ "stroke",                 _svg_style_parse_stroke_paint,          "none" },
	{ "stroke-width",           _svg_style_parse_stroke_width,          "1.0" },
	{ "text-anchor",            _svg_style_parse_text_anchor,           "start" },
/* XXX: { "text-rendering",	_svg_style_parse_text_rendering,	"auto" }, */
	{ "visibility",             _svg_style_parse_visibility,            "visible" },
/* XXX: { "word-spacing",	_svg_style_parse_word_spacing,		"normal" }, */
/* XXX: { "writing-mode",	_svg_style_parse_writing_mode,		"lr-tb" }, */
	{ "stop-opacity",           _svg_style_parse_stop_opacity,                  "1.0" },
	{ "stop-color",             _svg_style_parse_stop_color,                    "#ffffff" },
};

svg_status_t _svg_style_init_defaults(svg_style_t * style, svg_t * svg)
{
	style->svg = svg;
	for(int i = 0; i < SIZEOFARRAY(SVG_STYLE_PARSE_MAP); i++) {
		const svg_style_parse_map_t * map = &SVG_STYLE_PARSE_MAP[i];
		if(map->default_value) {
			svg_status_t status = (map->parse)(style, map->default_value);
			if(status)
				return status;
		}
	}
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_str_to_opacity(const char * str, double * ret)
{
	const char * end_ptr;
	double opacity = _svg_ascii_strtod(str, &end_ptr);
	if(end_ptr == str)
		return SVG_STATUS_PARSE_ERROR;
	if(end_ptr && end_ptr[0] == '%')
		opacity *= 0.01;
	*ret = opacity;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_color(svg_style_t * style, const char * str)
{
	if(strcmp(str, "inherit") != 0) {
		svg_status_t status = _svg_color_init_from_str(&style->color, str);
		if(status)
			return status;
		style->flags |= SVG_STYLE_FLAG_COLOR;
	}
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_fill_opacity(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_style_str_to_opacity(str, &style->fill_opacity);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_FILL_OPACITY;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_fill_paint(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_paint_init(&style->fill_paint, style->svg, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_FILL_PAINT;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_fill_rule(svg_style_t * style, const char * str)
{
	if(strcmp(str, "evenodd") == 0)
		style->fill_rule = SVG_FILL_RULE_EVEN_ODD;
	else if(strcmp(str, "nonzero") == 0)
		style->fill_rule = SVG_FILL_RULE_NONZERO;
	else
		/* XXX: Check SVG spec. for error name conventions */
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_FILL_RULE;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_font_family(svg_style_t * style, const char * str)
{
	free(style->font_family);
	style->font_family = strdup(str);
	if(style->font_family == NULL)
		return SVG_STATUS_NO_MEMORY;
	style->flags |= SVG_STYLE_FLAG_FONT_FAMILY;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_font_size(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_length_init_from_str(&style->font_size, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_FONT_SIZE;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_font_style(svg_style_t * style, const char * str)
{
	if(strcmp(str, "normal") == 0)
		style->font_style = SVG_FONT_STYLE_NORMAL;
	else if(strcmp(str, "italic") == 0)
		style->font_style = SVG_FONT_STYLE_ITALIC;
	else if(strcmp(str, "oblique") == 0)
		style->font_style = SVG_FONT_STYLE_OBLIQUE;
	else
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_FONT_STYLE;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_font_weight(svg_style_t * style, const char * str)
{
	if(strcmp(str, "normal") == 0)
		style->font_weight = 400;
	else if(strcmp(str, "bold") == 0)
		style->font_weight = 700;
	else if(strcmp(str, "lighter") == 0)
		style->font_weight -= 100;
	else if(strcmp(str, "bolder") ==0)
		style->font_weight += 100;
	else
		style->font_weight = (uint)_svg_ascii_strtod(str, NULL);
	if(style->font_weight < 100)
		style->font_weight = 100;
	if(style->font_weight > 900)
		style->font_weight = 900;
	style->flags |= SVG_STYLE_FLAG_FONT_WEIGHT;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_opacity(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_style_str_to_opacity(str, &style->opacity);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_OPACITY;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_dash_array(svg_style_t * style, const char * str)
{
	svgint_status_t status;
	double * new_dash_array;
	const char * end;
	int i, j;
	free(style->stroke_dash_array);
	style->num_dashes = 0;
	if(strcmp(str, "none") == 0) {
		style->flags |= SVG_STYLE_FLAG_STROKE_DASH_ARRAY;
		return SVG_STATUS_SUCCESS;
	}
	status = _svg_str_parse_all_csv_doubles(str, &style->stroke_dash_array, &style->num_dashes, &end);
	if(status)
		return (svg_status_t)status;
	if(style->num_dashes%2) {
		style->num_dashes *= 2;
		new_dash_array = (double *)realloc(style->stroke_dash_array, style->num_dashes*sizeof(double));
		if(new_dash_array == NULL)
			return SVG_STATUS_NO_MEMORY;
		style->stroke_dash_array = new_dash_array;
		for(i = 0, j = style->num_dashes/2; j < style->num_dashes; i++, j++)
			style->stroke_dash_array[j] = style->stroke_dash_array[i];
	}
	style->flags |= SVG_STYLE_FLAG_STROKE_DASH_ARRAY;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_dash_offset(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_length_init_from_str(&style->stroke_dash_offset, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_STROKE_DASH_OFFSET;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_line_cap(svg_style_t * style, const char * str)
{
	if(strcmp(str, "butt") == 0)
		style->stroke_line_cap = SVG_STROKE_LINE_CAP_BUTT;
	else if(strcmp(str, "round") == 0)
		style->stroke_line_cap = SVG_STROKE_LINE_CAP_ROUND;
	else if(strcmp(str, "square") == 0)
		style->stroke_line_cap = SVG_STROKE_LINE_CAP_SQUARE;
	else
		/* XXX: Check SVG spec. for error name conventions */
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_STROKE_LINE_CAP;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_line_join(svg_style_t * style, const char * str)
{
	if(strcmp(str, "miter") == 0)
		style->stroke_line_join = SVG_STROKE_LINE_JOIN_MITER;
	else if(strcmp(str, "round") == 0)
		style->stroke_line_join = SVG_STROKE_LINE_JOIN_ROUND;
	else if(strcmp(str, "bevel") == 0)
		style->stroke_line_join = SVG_STROKE_LINE_JOIN_BEVEL;
	else
		/* XXX: Check SVG spec. for error name conventions */
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_STROKE_LINE_JOIN;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_miter_limit(svg_style_t * style, const char * str)
{
	const char * end;
	style->stroke_miter_limit = _svg_ascii_strtod(str, &end);
	if(end == (char *)str)
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_STROKE_MITER_LIMIT;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_opacity(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_style_str_to_opacity(str, &style->stroke_opacity);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_STROKE_OPACITY;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_paint(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_paint_init(&style->stroke_paint, style->svg, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_STROKE_PAINT;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stroke_width(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_length_init_from_str(&style->stroke_width, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_STROKE_WIDTH;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_text_anchor(svg_style_t * style, const char * str)
{
	if(strcmp(str, "start") == 0)
		style->text_anchor = SVG_TEXT_ANCHOR_START;
	else if(strcmp(str, "middle") == 0)
		style->text_anchor = SVG_TEXT_ANCHOR_MIDDLE;
	else if(strcmp(str, "end") == 0)
		style->text_anchor = SVG_TEXT_ANCHOR_END;
	else
		return SVG_STATUS_PARSE_ERROR;
	style->flags |= SVG_STYLE_FLAG_TEXT_ANCHOR;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_visibility(svg_style_t * style, const char * str)
{
	/* XXX: Do we care about the CSS2 definitions for these? */
	if(strcmp(str, "hidden") == 0 || strcmp(str, "collapse") == 0)
		style->flags &= ~SVG_STYLE_FLAG_VISIBILITY;
	else if(strcmp(str, "visible") == 0)
		style->flags |= SVG_STYLE_FLAG_VISIBILITY;
	else if(strcmp(str, "inherit") != 0)
		return SVG_STATUS_PARSE_ERROR;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_display(svg_style_t * style, const char * str)
{
	/* XXX: Do we care about the CSS2 definitions for these? */
	if(strcmp(str, "none") == 0)
		style->flags &= ~SVG_STYLE_FLAG_DISPLAY;
	else if(strcmp(str, "inline") == 0 || strcmp(str, "block") == 0 ||
	        strcmp(str, "list-item") == 0 || strcmp(str, "run-in") == 0 ||
	        strcmp(str, "compact") == 0 || strcmp(str, "marker") == 0 ||
	        strcmp(str, "table") == 0 || strcmp(str, "inline-table") == 0 ||
	        strcmp(str, "table-row-group") == 0 || strcmp(str, "table-header-group") == 0 ||
	        strcmp(str, "table-footer-group") == 0 || strcmp(str, "table-row") == 0 ||
	        strcmp(str, "table-column-group") == 0 || strcmp(str, "table-column") == 0 ||
	        strcmp(str, "table-cell") == 0 || strcmp(str, "table-caption") == 0)
		style->flags |= SVG_STYLE_FLAG_DISPLAY;
	else if(strcmp(str, "inherit") != 0)
		return SVG_STATUS_PARSE_ERROR;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stop_color(svg_style_t * style, const char * str)
{
	svg_status_t status = _svg_color_init_from_str(&style->color, str);
	if(status)
		return status;
	style->flags |= SVG_STYLE_FLAG_COLOR;
	return SVG_STATUS_SUCCESS;
}

static svg_status_t _svg_style_parse_stop_opacity(svg_style_t * style, const char * str)
{
	double opacity = 1.0;
	svg_status_t status = _svg_style_str_to_opacity(str, &opacity);
	if(status)
		return status;
	else {
		style->opacity = opacity;
		style->flags |= SVG_STYLE_FLAG_OPACITY;
		return SVG_STATUS_SUCCESS;
	}
}

static svg_status_t _svg_style_split_nv_pair_alloc(const char * nv_pair, char ** name, char ** value)
{
	const char * v_start;
	*name = strdup(nv_pair);
	if(*name == NULL)
		return SVG_STATUS_NO_MEMORY;
	char * colon = strchr(*name, ':');
	if(colon == NULL) {
		free(*name);
		*name = NULL;
		*value = NULL;
		return SVG_STATUS_PARSE_ERROR;
	}
	*colon = '\0';
	v_start = nv_pair+(colon-(char *)*name)+1;
	while(_svg_ascii_isspace(*v_start))
		v_start++;
	*value = strdup(v_start);
	return (*value == NULL) ? SVG_STATUS_NO_MEMORY : SVG_STATUS_SUCCESS;
}

/* Parse a CSS2 style argument */
static svg_status_t _svg_style_parse_nv_pair(svg_style_t * style, const char * nv_pair)
{
	uint i;
	char * name, * value;
	svg_status_t status = _svg_style_split_nv_pair_alloc(nv_pair, &name, &value);
	if(status)
		return status;
	/* guilty until proven innocent */
	/* XXX: Check SVG spec. for this error condition */
	status = SVG_STATUS_PARSE_ERROR;
	for(i = 0; i < SIZEOFARRAY(SVG_STYLE_PARSE_MAP); i++)
		if(strcmp(SVG_STYLE_PARSE_MAP[i].name, name) == 0) {
			status = (SVG_STYLE_PARSE_MAP[i].parse)(style, value);
			break;
		}
	free(name);
	free(value);
	return status;
}

/* This next function is:

   Copyright © 2000 Eazel, Inc.
   Copyright © 2002 Dom Lachowicz <cinamod@hotmail.com>
   Copyright © 2002 USC/Information Sciences Institute

   Author: Raph Levien <raph@artofcode.com>
 */
/* Parse a complete CSS2 style string into individual name/value
   pairs.

   XXX: It's known that this is _way_ out of spec. A more complete
   CSS2 implementation will happen later.
 */
static svg_status_t _svg_style_parse_style_str(svg_style_t * style, const char * str)
{
	int end;
	char * nv_pair;
	int start = 0;
	while(str[start] != '\0') {
		for(end = start; str[end] != '\0' && str[end] != ';'; end++) ;
		nv_pair = (char *)malloc(1+end-start);
		if(nv_pair == NULL)
			return SVG_STATUS_NO_MEMORY;
		memcpy(nv_pair, str+start, end-start);
		nv_pair[end-start] = '\0';
		_svg_style_parse_nv_pair(style, nv_pair);
		free(nv_pair);
		start = end;
		if(str[start] == ';') start++;
		while(str[start] == ' ') start++;
	}
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_style_apply_attributes(svg_style_t * style, const char ** attributes)
{
	const char * style_str, * str;
	_svg_attribute_get_string(attributes, "style", &style_str, NULL);
	if(style_str) {
		svg_status_t status = _svg_style_parse_style_str(style, style_str);
		if(status)
			return status;
	}
	for(uint i = 0; i < SIZEOFARRAY(SVG_STYLE_PARSE_MAP); i++) {
		const svg_style_parse_map_t * map = &SVG_STYLE_PARSE_MAP[i];
		_svg_attribute_get_string(attributes, map->name, &str, NULL);
		if(str) {
			svg_status_t status = (map->parse)(style, str);
			if(status)
				return status;
		}
	}
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_style_render(svg_style_t * style, svg_render_engine_t * engine, void * closure)
{
	svg_status_t status;
	if(style->flags&SVG_STYLE_FLAG_COLOR) {
		status = (engine->set_color)(closure, &style->color);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FILL_OPACITY) {
		status = (engine->set_fill_opacity)(closure, style->fill_opacity);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FILL_PAINT) {
		status = (engine->set_fill_paint)(closure, &style->fill_paint);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FILL_RULE) {
		status = (engine->set_fill_rule)(closure, style->fill_rule);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FONT_FAMILY) {
		status = (engine->set_font_family)(closure, style->font_family);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FONT_SIZE) {
		/* XXX: How to deal with units of USize ? */
		status = (engine->set_font_size)(closure, style->font_size);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FONT_STYLE) {
		status = (engine->set_font_style)(closure, style->font_style);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_FONT_WEIGHT) {
		status = (engine->set_font_weight)(closure, style->font_weight);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_OPACITY) {
		status = (engine->set_opacity)(closure, style->opacity);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_DASH_ARRAY) {
		/* XXX: How to deal with units of USize ? */
		status = (engine->set_stroke_dash_array)(closure, style->stroke_dash_array, style->num_dashes);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_DASH_OFFSET) {
		status = (engine->set_stroke_dash_offset)(closure, &style->stroke_dash_offset);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_LINE_CAP) {
		status = (engine->set_stroke_line_cap)(closure, style->stroke_line_cap);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_LINE_JOIN) {
		status = (engine->set_stroke_line_join)(closure, style->stroke_line_join);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_MITER_LIMIT) {
		status = (engine->set_stroke_miter_limit)(closure, style->stroke_miter_limit);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_OPACITY) {
		status = (engine->set_stroke_opacity)(closure, style->stroke_opacity);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_PAINT) {
		status = (engine->set_stroke_paint)(closure, &style->stroke_paint);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_STROKE_WIDTH) {
		status = (engine->set_stroke_width)(closure, &style->stroke_width);
		if(status)
			return status;
	}
	if(style->flags&SVG_STYLE_FLAG_TEXT_ANCHOR) {
		status = (engine->set_text_anchor)(closure, style->text_anchor);
		if(status)
			return status;
	}
	return SVG_STATUS_SUCCESS;
}

svg_status_t _svg_style_get_display(svg_style_t * style)
{
	return (style->flags&SVG_STYLE_FLAG_DISPLAY) ? SVG_STATUS_SUCCESS : SVG_STATUS_INVALID_VALUE;
}

svg_status_t _svg_style_get_visibility(svg_style_t * style)
{
	return (style->flags&SVG_STYLE_FLAG_VISIBILITY) ? SVG_STATUS_SUCCESS : SVG_STATUS_INVALID_VALUE;
}
//
// svg_ascii
//
static const uint16 svg_ascii_table_data[256] = {
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
	0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
	0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
	0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
	0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
	0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
	0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
	0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
	/* the upper 128 are all zeroes */
};

const uint16 * const svg_ascii_table = svg_ascii_table_data;

/**
 * svg_ascii_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 *
 * Converts a string to a #double value.
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration files or
 * other non-user input that should be locale independent.  To handle
 * input from the user you should normally use the locale-sensitive
 * system strtod() function.
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 *
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #double value.
 **/
double _svg_ascii_strtod(const char * nptr, const char ** endptr)
{
	char * fail_pos;
	double val;
	struct lconv * locale_data;
	const char * decimal_point;
	int decimal_point_len;
	const char * p, * decimal_point_pos;
	const char * end = NULL; /* Silence gcc */
	if(nptr == NULL)
		return 0;
	fail_pos = NULL;
	locale_data = localeconv();
	decimal_point = locale_data->decimal_point;
	decimal_point_len = strlen(decimal_point);
	decimal_point_pos = NULL;
	if(decimal_point[0] != '.' || decimal_point[1] != 0) {
		p = nptr;
		/* Skip leading space */
		while(_svg_ascii_isspace(*p))
			p++;
		/* Skip leading optional sign */
		if(*p == '+' || *p == '-')
			p++;
		if(p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
			p += 2;
			/* HEX - find the (optional) decimal point */

			while(_svg_ascii_isxdigit(*p))
				p++;
			if(*p == '.') {
				decimal_point_pos = p++;

				while(_svg_ascii_isxdigit(*p))
					p++;
				if(*p == 'p' || *p == 'P')
					p++;
				if(*p == '+' || *p == '-')
					p++;
				while(_svg_ascii_isdigit(*p))
					p++;
				end = p;
			}
		}
		else {
			while(_svg_ascii_isdigit(*p))
				p++;
			if(*p == '.') {
				decimal_point_pos = p++;

				while(_svg_ascii_isdigit(*p))
					p++;
				if(*p == 'e' || *p == 'E')
					p++;
				if(*p == '+' || *p == '-')
					p++;
				while(_svg_ascii_isdigit(*p))
					p++;
				end = p;
			}
		}
		/* For the other cases, we need not convert the decimal point */
	}
	/* Set errno to zero, so that we can distinguish zero results
	   and underflows */
	errno = 0;
	if(decimal_point_pos) {
		/* We need to convert the '.' to the locale specific decimal point */
		char * copy = (char *)malloc(end-nptr+1+decimal_point_len);
		char * c = copy;
		memcpy(c, nptr, decimal_point_pos-nptr);
		c += decimal_point_pos-nptr;
		memcpy(c, decimal_point, decimal_point_len);
		c += decimal_point_len;
		memcpy(c, decimal_point_pos+1, end-(decimal_point_pos+1));
		c += end-(decimal_point_pos+1);
		*c = 0;
		val = strtod(copy, &fail_pos);
		if(fail_pos) {
			if(fail_pos > decimal_point_pos)
				fail_pos = (char *)nptr+(fail_pos-copy)-(decimal_point_len-1);
			else
				fail_pos = (char *)nptr+(fail_pos-copy);
		}
		free(copy);

	}
	else
		val = strtod(nptr, &fail_pos);
	if(endptr)
		*endptr = fail_pos;
	return val;
}
