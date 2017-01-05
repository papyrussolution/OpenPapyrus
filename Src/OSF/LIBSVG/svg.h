/* svg.h: Public interface for libsvg

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

#ifndef SVG_H
#define SVG_H

#include <slib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct svg_t;

typedef struct svg_group svg_group_t;
typedef struct svg_element svg_element_t;
/*
	XXX: I'm still not convinced I want to export this structure
*/
struct svg_color_t : public SColorBase {
	int is_current_color;
};

/*
struct svg_rect_t {
	double x;
	double y;
	double width;
	double height;
};

typedef enum svg_preserve_aspect_ratio {
	SVG_PRESERVE_ASPECT_RATIO_UNKNOWN,
	SVG_PRESERVE_ASPECT_RATIO_NONE,
	SVG_PRESERVE_ASPECT_RATIO_XMINYMIN,
	SVG_PRESERVE_ASPECT_RATIO_XMIDYMIN,
	SVG_PRESERVE_ASPECT_RATIO_XMAXYMIN,
	SVG_PRESERVE_ASPECT_RATIO_XMINYMID,
	SVG_PRESERVE_ASPECT_RATIO_XMIDYMID,
	SVG_PRESERVE_ASPECT_RATIO_XMAXYMID,
	SVG_PRESERVE_ASPECT_RATIO_XMINYMAX,
	SVG_PRESERVE_ASPECT_RATIO_XMIDYMAX,
	SVG_PRESERVE_ASPECT_RATIO_XMAXYMAX
} svg_preserve_aspect_ratio_t;

typedef enum svg_meet_or_slice {
	SVG_MEET_OR_SLICE_UNKNOWN,
	SVG_MEET_OR_SLICE_MEET,
	SVG_MEET_OR_SLICE_SLICE
} svg_meet_or_slice_t;
*/

struct svg_view_box_t {
	svg_view_box_t()
	{
		AspectRatio = pasUnkn;
		MeetOrSlice = mosUnkn;
	}
	int    ParseAspectRatio(const char * pStr);
	int    ParseBox(const char * pStr);
	//
	// Preserve aspect ratio
	//
	enum {
		pasUnkn,
		pasNone,
		pasXMINYMIN,
		pasXMIDYMIN,
		pasXMAXYMIN,
		pasXMINYMID,
		pasXMIDYMID,
		pasXMAXYMID,
		pasXMINYMAX,
		pasXMIDYMAX,
		pasXMAXYMAX
	};
	enum {
		mosUnkn,
		mosMeet,
		mosSlice
	};
	FRect  Box;
	int16  AspectRatio; // pasXXX
	int16  MeetOrSlice; // mosXXX
	//svg_preserve_aspect_ratio_t aspect_ratio : 4;
	//svg_meet_or_slice_t meet_or_slice : 2;
};

/* XXX: I want to think very carefully about the names of these error
   messages. First, they should follow the SVG spec. closely. Second,
   which errors do we really want? */
typedef enum svg_status {
	SVG_STATUS_SUCCESS = 0,
	SVG_STATUS_NO_MEMORY,
	SVG_STATUS_IO_ERROR,
	SVG_STATUS_FILE_NOT_FOUND,
	SVG_STATUS_INVALID_VALUE,
	SVG_STATUS_INVALID_CALL,
	SVG_STATUS_PARSE_ERROR
} svg_status_t;

typedef enum svg_fill_rule {
	SVG_FILL_RULE_NONZERO,
	SVG_FILL_RULE_EVEN_ODD
} svg_fill_rule_t;

typedef enum svg_font_style {
	SVG_FONT_STYLE_NORMAL,
	SVG_FONT_STYLE_ITALIC,
	SVG_FONT_STYLE_OBLIQUE
} svg_font_style_t;

typedef enum svg_stroke_line_cap {
	SVG_STROKE_LINE_CAP_BUTT,
	SVG_STROKE_LINE_CAP_ROUND,
	SVG_STROKE_LINE_CAP_SQUARE
} svg_stroke_line_cap_t;

typedef enum svg_stroke_line_join {
	SVG_STROKE_LINE_JOIN_BEVEL,
	SVG_STROKE_LINE_JOIN_MITER,
	SVG_STROKE_LINE_JOIN_ROUND
} svg_stroke_line_join_t;

typedef enum svg_text_anchor {
	SVG_TEXT_ANCHOR_START,
	SVG_TEXT_ANCHOR_MIDDLE,
	SVG_TEXT_ANCHOR_END
} svg_text_anchor_t;

typedef enum svg_gradient_type_t {
	SVG_GRADIENT_LINEAR,
	SVG_GRADIENT_RADIAL
} svg_gradient_type_t;

typedef enum svg_gradient_units {
	SVG_GRADIENT_UNITS_USER,
	SVG_GRADIENT_UNITS_BBOX
} svg_gradient_units_t;

typedef enum svg_gradient_spread {
	SVG_GRADIENT_SPREAD_PAD,
	SVG_GRADIENT_SPREAD_REPEAT,
	SVG_GRADIENT_SPREAD_REFLECT
} svg_gradient_spread_t;

class _XmlAttributes {
public:
	_XmlAttributes(const char ** ppAttrList);
	operator const char ** ();
	int GetStr(const char * pName, const char * pDef, SString & rBuf);
	int GetReal(const char * pName, double def, double & rVal);
	int GetUSize(const char * pName, const char * pDef, USize & rVal);
private:
	const char * Find(const char * pName);
	const char ** PP_List;
};

struct SvgGradientStop {
	svg_color_t color;
	double offset;
	double opacity;
};

struct SvgGradient {
	svg_status_t Init();
	svg_status_t Init(SvgGradient * other);
	svg_status_t Deinit();
	svg_status_t SetType(svg_gradient_type_t type);
	svg_status_t AddStop(double offset, svg_color_t * color, double opacity);
	svg_status_t ApplyAttributes(svg_t * svg, _XmlAttributes & rAttrList);

	svg_gradient_type_t type;
	union {
		struct {
			USize x1;
			USize y1;
			USize x2;
			USize y2;
		} linear;
		struct {
			USize cx;
			USize cy;
			USize r;
			USize fx;
			USize fy;
		} radial;
	} u;
	svg_gradient_units_t units;
	svg_gradient_spread_t spread;
	LMatrix2D transform;
	SvgGradientStop * stops;
	int num_stops;
	int stops_size;
};

enum svg_pattern_units_t {
	SVG_PATTERN_UNITS_USER,
	SVG_PATTERN_UNITS_BBOX
};

/* XXX: I'm still not convinced I want to export this structure */
struct svg_pattern_t {
	svg_element_t * group_element;
	svg_pattern_units_t units;
	svg_pattern_units_t content_units;
	USize x, y, width, height;
	LMatrix2D transform;
};

/* XXX: I'm still not convinced I want to export this structure */
/* XXX: This needs to be fleshed out considerably more than this */
enum svg_paint_type_t {
	SVG_PAINT_TYPE_NONE,
	SVG_PAINT_TYPE_COLOR,
	SVG_PAINT_TYPE_GRADIENT,
	SVG_PAINT_TYPE_PATTERN
};

/*
	XXX: I'm still not convinced I want to export this structure
*/
struct svg_paint_t {
	svg_paint_type_t type;
	union {
		svg_color_t color;
		SvgGradient * gradient;
		svg_element_t * pattern_element;
	} p;
};
/* 
	XXX: Here's another piece of the API that needs deep consideration. 
*/
struct svg_render_engine_t {
	/* 
		hierarchy 
	*/
	svg_status_t (* begin_group)(void * closure, double opacity);
	svg_status_t (* begin_element)(void * closure);
	svg_status_t (* end_element)(void * closure);
	svg_status_t (* end_group)(void * closure, double opacity);
	/* 
		path creation 
	*/
	svg_status_t (* move_to)(void * closure, double x, double y);
	svg_status_t (* line_to)(void * closure, double x, double y);
	svg_status_t (* curve_to)(void * closure, double x1, double y1, double x2, double y2, double x3, double y3);
	svg_status_t (* quadratic_curve_to)(void * closure, double x1, double y1, double x2, double y2);
	svg_status_t (* arc_to)(void * closure, double rx, double ry, double x_axis_rotation, int large_arc_flag,
		int sweep_flag, double x, double y);
	svg_status_t (* close_path)(void * closure);
	/* 
		style 
	*/
	svg_status_t (* set_color)(void * closure, const svg_color_t * color);
	svg_status_t (* set_fill_opacity)(void * closure, double fill_opacity);
	svg_status_t (* set_fill_paint)(void * closure, const svg_paint_t * paint);
	svg_status_t (* set_fill_rule)(void * closure, svg_fill_rule_t fill_rule);
	svg_status_t (* set_font_family)(void * closure, const char * family);
	svg_status_t (* set_font_size)(void * closure, double size);
	svg_status_t (* set_font_style)(void * closure, svg_font_style_t font_style);
	svg_status_t (* set_font_weight)(void * closure, uint font_weight);
	svg_status_t (* set_opacity)(void * closure, double opacity);
	svg_status_t (* set_stroke_dash_array)(void * closure, double * dash_array, int num_dashes);
	svg_status_t (* set_stroke_dash_offset)(void * closure, USize * offset);
	svg_status_t (* set_stroke_line_cap)(void * closure, svg_stroke_line_cap_t line_cap);
	svg_status_t (* set_stroke_line_join)(void * closure, svg_stroke_line_join_t line_join);
	svg_status_t (* set_stroke_miter_limit)(void * closure, double limit);
	svg_status_t (* set_stroke_opacity)(void * closure, double stroke_opacity);
	svg_status_t (* set_stroke_paint)(void * closure, const svg_paint_t * paint);
	svg_status_t (* set_stroke_width)(void * closure, USize * width);
	svg_status_t (* set_text_anchor)(void * closure, svg_text_anchor_t text_anchor);
	/* 
		transform 
	*/
	svg_status_t (* transform)(void * closure, double a, double b, double c, double d, double e, double f);
	svg_status_t (* apply_view_box)(void * closure, svg_view_box_t view_box, USize * width, USize * height);
	svg_status_t (* set_viewport_dimension)(void * closure, USize * width, USize * height);
	/* 
		drawing 
	*/
	svg_status_t (* render_line)(void * closure, USize * x1, USize * y1, USize * x2, USize * y2);
	svg_status_t (* render_path)(void * closure);
	svg_status_t (* render_ellipse)(void * closure, USize * cx, USize * cy, USize * rx, USize * ry);
	svg_status_t (* render_rect)(void * closure, USize * x, USize * y, USize * width, USize * height, USize * rx, USize * ry);
	svg_status_t (* render_text)(void * closure, USize * x, USize * y, const char * utf8);
	svg_status_t (* render_image)(void * closure, uchar * data, uint data_width,
		uint data_height, USize * x, USize * y, USize * width, USize * height);
};

struct svg_parser_t;

//typedef svg_status_t (svg_parser_parse_element_t)(svg_parser_t * parser, const char ** attributes, svg_element_t ** element_ret);
//typedef svg_status_t (svg_parser_parse_characters_t)(svg_parser_t * parser, const char * ch, int len);

struct svg_parser_cb_t {
	svg_status_t (* parse_element)(svg_parser_t * parser, const char ** attributes, svg_element_t ** element_ret);
	svg_status_t (* parse_characters)(svg_parser_t * parser, const char * ch, int len);
	//svg_parser_parse_element_t * parse_element;
	//svg_parser_parse_characters_t * parse_characters;
};

enum svg_element_type_t {
	SVG_ELEMENT_TYPE_SVG_GROUP,
	SVG_ELEMENT_TYPE_GROUP,
	SVG_ELEMENT_TYPE_DEFS,
	SVG_ELEMENT_TYPE_USE,
	SVG_ELEMENT_TYPE_SYMBOL,
	SVG_ELEMENT_TYPE_PATH,
	SVG_ELEMENT_TYPE_CIRCLE,
	SVG_ELEMENT_TYPE_ELLIPSE,
	SVG_ELEMENT_TYPE_LINE,
	SVG_ELEMENT_TYPE_RECT,
	SVG_ELEMENT_TYPE_TEXT,
	SVG_ELEMENT_TYPE_GRADIENT,
	SVG_ELEMENT_TYPE_GRADIENT_STOP,
	SVG_ELEMENT_TYPE_PATTERN,
	SVG_ELEMENT_TYPE_IMAGE
};

struct svg_parser_t {
	struct State {
		const svg_parser_cb_t * cb;
		svg_element_t * group_element;
		SString Txt;
		SString Tag; // Наименование тэга, по которому произошло сохранение состояния //
		State * next;
	};
	svg_parser_t()
	{
		svg = 0;
		ctxt = 0;
		unknown_element_depth = 0;
		state = 0;
		status = SVG_STATUS_SUCCESS;
	}
	int    PushState(const char * pTag);
	int    PopState();
	int    NewLeafElement(svg_element_t ** child_element, svg_element_type_t type);
	int    NewGroupElement(svg_element_t ** group_element, svg_element_type_t type);

	svg_t * svg;
	/*svg_xml_parser_context_t*/void * ctxt; // Контекст XML-парсера
	uint unknown_element_depth;
	State * state;
	/*svg_xml_hash_table_t*/void * entities;
	svg_status_t status;
};

struct svg_t {
	svg_t();
	~svg_t();

	double dpi;
	char * dir_name;
	svg_element_t * group_element;
	/*svg_xml_hash_table_t*/void * element_ids;
	svg_parser_t parser;
	svg_render_engine_t * engine;
};

svg_status_t svg_parse(svg_t * svg, const char * filename);
svg_status_t svg_parse_file(svg_t * svg, FILE * file);
svg_status_t svg_parse_buffer(svg_t * svg, const char * buf, size_t count);
svg_status_t svg_parse_chunk_begin(svg_t * svg);
svg_status_t svg_parse_chunk(svg_t * svg, const char * buf, size_t count);
svg_status_t svg_parse_chunk_end(svg_t * svg);
svg_status_t svg_render(svg_t * svg, svg_render_engine_t * engine, void * closure);
void svg_get_size(svg_t * svg, USize * width, USize * height);
/* 
	svg_element 
*/
svg_status_t svg_element_render(svg_element_t * element, svg_render_engine_t * engine, void * closure);
svg_pattern_t * svg_element_pattern(svg_element_t * element);

#ifdef __cplusplus
}
#endif

#endif
