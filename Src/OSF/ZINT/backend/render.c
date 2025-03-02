/*
 * render.c - Generic Rendered Format
 *
 * Initiall written by Sam Lown for use in gLabels. Converts encoded
 * data into a generic internal structure of lines and characters
 * usable in external applications.
 */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.
 */
#include "common.h"
#pragma hdrstop

#define GL_CONST  2.8346f

struct zint_render_line * render_plot_create_line(float x, float y, float width, float length);
int render_plot_add_line(struct ZintSymbol * symbol, struct zint_render_line * line, struct zint_render_line ** last_line);
struct zint_render_ring * render_plot_create_ring(float x, float y, float radius, float line_width);
int render_plot_add_ring(struct ZintSymbol * symbol, struct zint_render_ring * ring, struct zint_render_ring ** last_ring);
struct zint_render_hexagon * render_plot_create_hexagon(float x, float y);
int render_plot_add_hexagon(struct ZintSymbol * symbol, struct zint_render_hexagon * ring, struct zint_render_hexagon ** last_hexagon);

int render_plot_add_string(struct ZintSymbol * symbol, uchar * text, float x, float y, float fsize, float width, struct zint_render_string ** last_string);

int render_plot(struct ZintSymbol * symbol, const float width, const float height)
{
	struct zint_render * render;
	struct zint_render_line * line, * last_line = NULL;
	struct zint_render_string * last_string = NULL;
	struct zint_render_ring * ring, * last_ring = NULL;
	struct zint_render_hexagon * hexagon, * last_hexagon = NULL;
	int i, r, block_width, latch, this_row;
	float textpos, textwidth, large_bar_height, preset_height, row_height, row_posn = 0.0;
	// int error_number = 0;
	int text_offset, text_height, xoffset, yoffset, textdone, main_symbol_width_x, addon_width_x;
	char addon[6], textpart[10];
	int large_bar_count, symbol_lead_in, total_symbol_width_x, total_area_width_x;
	float addon_text_posn;
	float default_text_posn;
	float scaler;
	const char * locale = NULL;
	int hide_text = 0;
	float required_aspect;
	float symbol_aspect = 1;
	float x_dimension;
	int upceanflag = 0;
	// Allocate memory for the rendered version
	render = symbol->rendered = (struct zint_render*)SAlloc::M(sizeof(struct zint_render));
	if(!symbol->rendered) 
		return ZINT_ERROR_MEMORY;
	render->lines = NULL;
	render->strings = NULL;
	render->rings = NULL;
	render->hexagons = NULL;
	locale = setlocale(LC_ALL, "C");
	row_height = 0;
	textdone = 0;
	textpos = 0.0;
	main_symbol_width_x = symbol->width;
	sstrcpy(addon, "");
	symbol_lead_in = 0;
	addon_text_posn = 0.0;
	addon_width_x = 0;

	/*
	 * Determine if there will be any addon texts and text height
	 */
	latch = 0;
	r = 0;
	/* Isolate add-on text */
	if(is_extendable(symbol->Std)) {
		const size_t stl_ = sstrlen(symbol->text);
		for(size_t sti = 0; sti < stl_; sti++) {
			if(latch == 1) {
				addon[r] = symbol->text[sti];
				r++;
			}
			if(symbol->text[sti] == '+')
				latch = 1;
		}
	}
	addon[r] = '\0';
	if(!symbol->show_hrt || isempty(symbol->text)) {
		hide_text = 1;
		text_height = text_offset = 0;
	}
	else {
		text_height = 9;
		text_offset = 2;
	}
	/*
	 * Calculate the width of the barcode, especially if there are any extra
	 * borders or white space to add.
	 */
	while(!(module_is_set(symbol, symbol->rows - 1, symbol_lead_in))) {
		symbol_lead_in++;
	}
	/* Certain symbols need whitespace otherwise characters get chopped off the sides */
	if((((symbol->Std == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->Std == BARCODE_EANX_CC)) || (symbol->Std == BARCODE_ISBNX)) {
		switch(sstrlen(symbol->text)) {
			case 13: /* EAN 13 */
			case 16:
			case 19:
			    if(symbol->whitespace_width == 0) {
				    symbol->whitespace_width = 10;
			    }
			    main_symbol_width_x = 96 + symbol_lead_in;
			    upceanflag = 13;
			    break;
			case 2:
			    main_symbol_width_x = 22 + symbol_lead_in;
			    upceanflag = 2;
			    break;
			case 5:
			    main_symbol_width_x = 49 + symbol_lead_in;
			    upceanflag = 5;
			    break;
			default:
			    main_symbol_width_x = 68 + symbol_lead_in;
			    upceanflag = 8;
		}
		switch(sstrlen(symbol->text)) {
			case 11:
			case 16:
			    /* EAN-2 add-on */
			    addon_width_x = 31;
			    break;
			case 14:
			case 19:
			    /* EAN-5 add-on */
			    addon_width_x = 58;
			    break;
		}
	}
	else if(((symbol->Std == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCA_CC)) {
		upceanflag = 12;
		if(symbol->whitespace_width < 10) {
			symbol->whitespace_width = 10;
			main_symbol_width_x = 96 + symbol_lead_in;
		}
		switch(sstrlen(symbol->text)) {
			case 15: /* EAN-2 add-on */
			    addon_width_x = 31;
			    break;
			case 18: /* EAN-5 add-on */
			    addon_width_x = 58;
			    break;
		}
	}
	else if(((symbol->Std == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCE_CC)) {
		upceanflag = 6;
		if(symbol->whitespace_width == 0) {
			symbol->whitespace_width = 10;
			main_symbol_width_x = 51 + symbol_lead_in;
		}
		switch(sstrlen(symbol->text)) {
			case 11: /* EAN-2 add-on */
			    addon_width_x = 31;
			    break;
			case 14: /* EAN-5 add-on */
			    addon_width_x = 58;
			    break;
		}
	}
	total_symbol_width_x = main_symbol_width_x + addon_width_x;
	total_area_width_x = total_symbol_width_x + (2 * (symbol->border_width + symbol->whitespace_width));
	xoffset = symbol->border_width + symbol->whitespace_width;
	yoffset = symbol->border_width;
	// Determine if height should be overridden
	large_bar_count = 0;
	preset_height = 0.0;
	for(i = 0; i < symbol->rows; i++) {
		preset_height += symbol->row_height[i];
		if(symbol->row_height[i] == 0) {
			large_bar_count++;
		}
	}
	if(large_bar_count == 0) {
		required_aspect = width / height;
		symbol_aspect = (total_symbol_width_x + (2 * xoffset)) / (preset_height + (2 * yoffset) + text_offset + text_height);
		symbol->height = (int)preset_height;
		if(required_aspect > symbol_aspect) {
			/* the area is too wide */
			scaler = height / (preset_height + (2 * yoffset) + text_offset + text_height);
			render->width = symbol_aspect * height;
			render->height = height;
		}
		else {
			/* the area is too high */
			scaler = width / (total_symbol_width_x + (2 * xoffset));
			render->width = width;
			render->height = width / symbol_aspect;
		}
	}
	else {
		scaler = width / (total_symbol_width_x + (2 * xoffset));
		symbol->height = (int)((height / scaler) - ((2 * yoffset) + text_offset + text_height));
		render->width = width;
		render->height = height;
	}
	large_bar_height = (symbol->height - preset_height) / large_bar_count;
	if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
		default_text_posn = (symbol->height + text_offset + symbol->border_width + symbol->border_width) * scaler;
	}
	else {
		default_text_posn = (symbol->height + text_offset + symbol->border_width) * scaler;
	}
	x_dimension = render->width / total_area_width_x;
	x_dimension /= GL_CONST;
	/* Set minimum size of symbol */
	/* Barcode must be at least 2mm high by 2mm across */
	if(render->height < ((x_dimension * ((2 * symbol->border_width) + text_offset + text_height)) + 2.0) * GL_CONST) {
		render->height = ((x_dimension * ((2 * symbol->border_width) + text_offset + text_height)) + 2.0f) * GL_CONST;
	}
	if(render->width < (2.0 * GL_CONST)) {
		render->width = (2.0f * GL_CONST);
	}
	if(symbol->Std == BARCODE_CODABAR) {
		/* The minimum X-dimension of Codabar is 0.191mm. The minimum bar height is 5mm */
		if(x_dimension < 0.191) {
			render->width = 0.191f * GL_CONST * total_area_width_x;
		}
		if(render->height < ((x_dimension * ((2 * symbol->border_width) + text_offset + text_height)) + 5.0) * GL_CONST) {
			render->height = ((x_dimension * ((2 * symbol->border_width) + text_offset + text_height)) + 5.0f) * GL_CONST;
		}
	}
	else if(symbol->Std == BARCODE_CODE49) {
		/* The minimum X-dimension of Code 49 is 0.191mm */
		if(x_dimension < 0.191) {
			render->width = 0.191f * GL_CONST * total_area_width_x;
			render->height = render->width / symbol_aspect;
		}
	}
	if(upceanflag != 0) {
		/* The X-dimension of UPC/EAN symbols is fixed at 0.330mm */
		/* NOTE: This code will need adjustment before it correctly deals with composite symbols */
		render->width = 0.330f * GL_CONST * total_area_width_x;
		/* The height is also fixed */
		switch(upceanflag) {
			case 6:
			case 12:
			case 13:
			    /* UPC-A, UPC-E and EAN-13 */
			    /* Height of bars should be 22.85mm */
			    render->height = ((0.330f * ((2 * symbol->border_width) + text_offset + text_height)) + 22.85f) * GL_CONST;
			    break;
			case 8:
			    /* EAN-8 */
			    /* Height of bars should be 18.23mm */
			    render->height = ((0.330f * ((2 * symbol->border_width) + text_offset + text_height)) + 18.23f) * GL_CONST;
			    break;
			default:
			    /* EAN-2 and EAN-5 */
			    /* Height of bars should be 21.10mm */
			    render->height = ((0.330f * ((2 * symbol->border_width) + text_offset + text_height)) + 21.10f) * GL_CONST;
		}
	}
	if(symbol->Std == BARCODE_ONECODE) {
		/* The size of USPS Intelligent Mail barcode is fixed */
		render->width = 0.508f * GL_CONST * total_area_width_x;
		render->height = 4.064f * GL_CONST;
	}
	else if((symbol->Std == BARCODE_POSTNET) || (symbol->Std == BARCODE_PLANET)) {
		/* The size of PostNet and PLANET are fized */
		render->width = 0.508f * GL_CONST * total_area_width_x;
		render->height = 2.921f * GL_CONST;
	}
	else if(((symbol->Std == BARCODE_AUSPOST) || (symbol->Std == BARCODE_AUSREPLY)) || ((symbol->Std == BARCODE_AUSROUTE) || (symbol->Std == BARCODE_AUSREDIRECT))) {
		/* Australia Post use the same sizes as USPS */
		render->width = 0.508f * GL_CONST * total_area_width_x;
		render->height = 4.064f * GL_CONST;
	}
	else if((symbol->Std == BARCODE_RM4SCC) || (symbol->Std == BARCODE_KIX)) {
		/* Royal Mail and KIX Code uses 22 bars per inch */
		render->width = 0.577f * GL_CONST * total_area_width_x;
		render->height = 5.22f * GL_CONST;
	}
	if(symbol->Std == BARCODE_MAXICODE) {
		/* Maxicode is a fixed size */
		scaler = GL_CONST; /* Converts from millimeters to the scale used by glabels */
		render->width = 28.16f * scaler;
		render->height = 26.86f * scaler;
		/* Central bullseye pattern */
		ring = render_plot_create_ring(13.64f * scaler, 13.43f * scaler, 0.85f * scaler, 0.67f * scaler);
		render_plot_add_ring(symbol, ring, &last_ring);
		ring = render_plot_create_ring(13.64f * scaler, 13.43f * scaler, 2.20f * scaler, 0.67f * scaler);
		render_plot_add_ring(symbol, ring, &last_ring);
		ring = render_plot_create_ring(13.64f * scaler, 13.43f * scaler, 3.54f * scaler, 0.67f * scaler);
		render_plot_add_ring(symbol, ring, &last_ring);
		/* Hexagons */
		for(r = 0; r < symbol->rows; r++) {
			for(i = 0; i < symbol->width; i++) {
				if(module_is_set(symbol, r, i)) {
					hexagon = render_plot_create_hexagon(((i * 0.88f) + ((r & 1) ? 1.76f : 1.32f)) * scaler, ((r * 0.76f) + 0.76f) * scaler);
					render_plot_add_hexagon(symbol, hexagon, &last_hexagon);
				}
			}
		}
	}
	else {
		/* everything else uses rectangles (or squares) */
		/* Works from the bottom of the symbol up */
		int addon_latch = 0;
		for(r = 0; r < symbol->rows; r++) {
			this_row = r;
			if(symbol->row_height[this_row] == 0) {
				row_height = large_bar_height;
			}
			else {
				row_height = (float)symbol->row_height[this_row];
			}
			row_posn = 0;
			for(i = 0; i < r; i++) {
				if(symbol->row_height[i] == 0) {
					row_posn += large_bar_height;
				}
				else {
					row_posn += symbol->row_height[i];
				}
			}
			row_posn += yoffset;
			i = 0;
			if(module_is_set(symbol, this_row, 0)) {
				latch = 1;
			}
			else {
				latch = 0;
			}
			do {
				block_width = 0;
				do {
					block_width++;
				} while(module_is_set(symbol, this_row, i + block_width) == module_is_set(symbol, this_row, i));
				if((addon_latch == 0) && (r == (symbol->rows - 1)) && (i > main_symbol_width_x)) {
					addon_text_posn = row_posn * scaler;
					addon_latch = 1;
				}
				if(latch == 1) {
					/* a bar */
					if(addon_latch == 0) {
						line = render_plot_create_line((i + xoffset) * scaler, (row_posn) * scaler, block_width * scaler, row_height * scaler);
					}
					else {
						line = render_plot_create_line((i + xoffset) * scaler, (row_posn + 10.0f) * scaler, block_width * scaler, (row_height - 5.0f) * scaler);
					}
					latch = 0;
					render_plot_add_line(symbol, line, &last_line);
				}
				else {
					/* a space */
					latch = 1;
				}
				i += block_width;
			} while(i < symbol->width);
		}
	}
	/* That's done the actual data area, everything else is human-friendly */

	/* Add the text */
	xoffset -= symbol_lead_in;
	row_posn = (row_posn + large_bar_height) * scaler;

	if(!hide_text) {
		if(upceanflag == 8) {
			/* guard bar extensions and text formatting for EAN-8 */
			i = 0;
			for(line = symbol->rendered->lines; line != NULL; line = line->next) {
				switch(i) {
					case 0:
					case 1:
					case 10:
					case 11:
					case 20:
					case 21:
					    line->length += (5.0f * scaler);
					    break;
				}
				i++;
			}
			for(i = 0; i < 4; i++) {
				textpart[i] = symbol->text[i];
			}
			textpart[4] = '\0';
			textpos = 17;
			textwidth = 4.0 * 8.5;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 4; i++) {
				textpart[i] = symbol->text[i + 4];
			}
			textpart[4] = '\0';
			textpos = 50;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			textdone = 1;
			switch(strlen(addon)) {
				case 2:
				    textpos = (float)(xoffset + 86);
				    textwidth = 2.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
				case 5:
				    textpos = (float)(xoffset + 100);
				    textwidth = 5.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
			}
		}

		if(upceanflag == 13) {
			/* guard bar extensions and text formatting for EAN-13 */
			i = 0;
			for(line = symbol->rendered->lines; line != NULL; line = line->next) {
				switch(i) {
					case 0:
					case 1:
					case 14:
					case 15:
					case 28:
					case 29:
					    line->length += (5.0f * scaler);
					    break;
				}
				i++;
			}

			textpart[0] = symbol->text[0];
			textpart[1] = '\0';
			textpos = -5; // 7
			textwidth = 8.5;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 6; i++) {
				textpart[i] = symbol->text[i+1];
			}
			textpart[6] = '\0';
			textpos = 25;
			textwidth = 6.0 * 8.5;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 6; i++) {
				textpart[i] = symbol->text[i + 7];
			}
			textpart[6] = '\0';
			textpos = 72;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			textdone = 1;
			switch(strlen(addon)) {
				case 2:
				    textpos = (float)(xoffset + 114);
				    textwidth = 2.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
				case 5:
				    textpos = (float)(xoffset + 128);
				    textwidth = 5.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler,	&last_string);
				    break;
			}
		}
		if(upceanflag == 12) {
			/* guard bar extensions and text formatting for UPCA */
			i = 0;
			for(line = symbol->rendered->lines; line != NULL; line = line->next) {
				switch(i) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 14:
					case 15:
					case 26:
					case 27:
					case 28:
					case 29:
					    line->length += (5.0f * scaler);
					    break;
				}
				i++;
			}

			textpart[0] = symbol->text[0];
			textpart[1] = '\0';
			textpos = -5;
			textwidth = 6.2f;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn + (2.0f * scaler),
			    8.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 5; i++) {
				textpart[i] = symbol->text[i+1];
			}
			textpart[5] = '\0';
			textpos = 27;
			textwidth = 5.0 * 8.5;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 5; i++) {
				textpart[i] = symbol->text[i + 6];
			}
			textpos = 68;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			textpart[0] = symbol->text[11];
			textpart[1] = '\0';
			textpos = 100;
			textwidth = 6.2f;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn + (2.0f * scaler),
			    8.0f * scaler, textwidth * scaler, &last_string);
			textdone = 1;
			switch(strlen(addon)) {
				case 2:
				    textpos = (float)(xoffset + 116);
				    textwidth = 2.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
				case 5:
				    textpos = (float)(xoffset + 130);
				    textwidth = 5.0f * 8.5f;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
			}
		}

		if(upceanflag == 6) {
			/* guard bar extensions and text formatting for UPCE */
			i = 0;
			for(line = symbol->rendered->lines; line != NULL; line = line->next) {
				switch(i) {
					case 0:
					case 1:
					case 14:
					case 15:
					case 16:
					    line->length += (5.0f * scaler);
					    break;
				}
				i++;
			}

			textpart[0] = symbol->text[0];
			textpart[1] = '\0';
			textpos = -5;
			textwidth = 6.2f;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn + (2.0f * scaler),
			    8.0f * scaler, textwidth * scaler, &last_string);
			for(i = 0; i < 6; i++) {
				textpart[i] = symbol->text[i+1];
			}
			textpart[6] = '\0';
			textpos = 24;
			textwidth = 6.0 * 8.5;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn,
			    11.0f * scaler, textwidth * scaler, &last_string);
			textpart[0] = symbol->text[7];
			textpart[1] = '\0';
			textpos = 55;
			textwidth = 6.2f;
			render_plot_add_string(symbol, (uchar *)textpart, (textpos + xoffset) * scaler, default_text_posn + (2.0f * scaler),
			    8.0f * scaler, textwidth * scaler, &last_string);
			textdone = 1;
			switch(strlen(addon)) {
				case 2:
				    textpos = (float)(xoffset + 70);
				    textwidth = 2.0 * 8.5;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
				case 5:
				    textpos = (float)(xoffset + 84);
				    textwidth = 5.0 * 8.5;
				    render_plot_add_string(symbol, (uchar *)addon, textpos * scaler, addon_text_posn * scaler,
						11.0f * scaler, textwidth * scaler, &last_string);
				    break;
			}
		}

		/* Put normal human readable text at the bottom (and centered) */
		if(textdone == 0) {
			// caculate start xoffset to center text
			render_plot_add_string(symbol, symbol->text, ((symbol->width / 2.0f) + xoffset) * scaler, default_text_posn,
			    9.0f * scaler, 0.0f, &last_string);
		}
	}
	switch(symbol->Std) {
		case BARCODE_MAXICODE:
		    /* Do nothing! */
		    break;
		default:
		    if((symbol->output_options & BARCODE_BIND) != 0) {
			    if((symbol->rows > 1) && (is_stackable(symbol->Std) == 1)) {
				    /* row binding */
				    for(r = 1; r < symbol->rows; r++) {
					    line = render_plot_create_line(xoffset * scaler, ((r * row_height) + yoffset - 1) * scaler,
							symbol->width * scaler, 2.0f * scaler);
					    render_plot_add_line(symbol, line, &last_line);
				    }
			    }
		    }
		    if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
			    line = render_plot_create_line(0,
			    0,
			    (symbol->width + xoffset + xoffset) * scaler,
			    symbol->border_width * scaler);
			    render_plot_add_line(symbol, line, &last_line);
			    line =
			    render_plot_create_line(0,
			    (symbol->height + symbol->border_width) * scaler,
			    (symbol->width + xoffset + xoffset) * scaler,
			    symbol->border_width * scaler);
			    render_plot_add_line(symbol, line, &last_line);
		    }
		    if(symbol->output_options & BARCODE_BOX) {
			    /* side bars */
			    line = render_plot_create_line(0,
			    0,
			    symbol->border_width * scaler,
			    (symbol->height + (2 * symbol->border_width)) * scaler);
			    render_plot_add_line(symbol, line, &last_line);
			    line = render_plot_create_line((symbol->width + xoffset + xoffset - symbol->border_width) * scaler,
			    0,
			    symbol->border_width * scaler,
			    (symbol->height + (2 * symbol->border_width)) * scaler);
			    render_plot_add_line(symbol, line, &last_line);
		    }
		    break;
	}

	if(locale)
		setlocale(LC_ALL, locale);

	return 1;
}

/*
 * Create a new line with its memory allocated ready for adding to the
 * rendered structure.
 *
 * This is much quicker than writing out each line manually (in some cases!)
 */
struct zint_render_line * render_plot_create_line(float x, float y, float width, float length)
{
	struct zint_render_line * line;

	line = (struct zint_render_line*)SAlloc::M(sizeof(struct zint_render_line));
	if(!line) return NULL;

	line->next = NULL;
	line->x = x;
	line->y = y;
	line->width = width;
	line->length = length;

	return line;
}

/*
 * Add the line to the current rendering and update the last line's
 * next value.
 */
int render_plot_add_line(struct ZintSymbol * symbol, struct zint_render_line * line, struct zint_render_line ** last_line)
{
	if(!line) return ZINT_ERROR_MEMORY;
	if(*last_line)
		(*last_line)->next = line;
	else
		symbol->rendered->lines = line;  // first line

	*last_line = line;
	return 1;
}

struct zint_render_ring * render_plot_create_ring(float x, float y, float radius, float line_width)
{
	struct zint_render_ring * ring;

	ring = (struct zint_render_ring*)SAlloc::M(sizeof(struct zint_render_ring));
	if(!ring) return NULL;
	ring->next = NULL;
	ring->x = x;
	ring->y = y;
	ring->radius = radius;
	ring->line_width = line_width;
	return ring;
}

int render_plot_add_ring(struct ZintSymbol * symbol, struct zint_render_ring * ring, struct zint_render_ring ** last_ring)
{
	if(!ring) return ZINT_ERROR_MEMORY;
	if(*last_ring)
		(*last_ring)->next = ring;
	else
		symbol->rendered->rings = ring;  // first ring
	*last_ring = ring;
	return 1;
}

struct zint_render_hexagon * render_plot_create_hexagon(float x, float y)
{
	struct zint_render_hexagon * hexagon = (struct zint_render_hexagon*)SAlloc::M(sizeof(struct zint_render_hexagon));
	if(!hexagon) return NULL;
	hexagon->next = NULL;
	hexagon->x = x;
	hexagon->y = y;
	return hexagon;
}

int render_plot_add_hexagon(struct ZintSymbol * symbol, struct zint_render_hexagon * hexagon, struct zint_render_hexagon ** last_hexagon)
{
	if(!hexagon) 
		return ZINT_ERROR_MEMORY;
	if(*last_hexagon)
		(*last_hexagon)->next = hexagon;
	else
		symbol->rendered->hexagons = hexagon;  // first hexagon
	*last_hexagon = hexagon;
	return 1;
}

/*
 * Add a string structure to the symbol.
 * Coordinates assumed to be from top-center.
 */
int render_plot_add_string(struct ZintSymbol * symbol, uchar * text, float x, float y, float fsize, float width,
    struct zint_render_string ** last_string)
{
	struct zint_render_string * string = (struct zint_render_string*)SAlloc::M(sizeof(struct zint_render_string));
	string->next = NULL;
	string->x = x;
	string->y = y;
	string->width = width;
	string->fsize = fsize;
	string->length = sstrlen(text);
	string->text = (uchar *)SAlloc::M(sizeof(uchar) * (sstrlen(text) + 1));
	sstrcpy(string->text, text);
	if(*last_string)
		(*last_string)->next = string;
	else
		symbol->rendered->strings = string;  // First character
	*last_string = string;
	return 1;
}

