// svg.c - Scalable Vector Graphics 
// libzint - the open source barcode library
// Copyright (C) 2009-2016 Robin Stuart <rstuart114@gmail.com>
//
#include "common.h"
#pragma hdrstop

#define SSET    "0123456789ABCDEF"

int svg_plot(struct ZintSymbol * symbol)
{
	int    block_width, latch, r, this_row;
	uint   i;
	float  textpos, large_bar_height, preset_height, row_height, row_posn = 0.0;
	FILE * fsvg;
	int error_number = 0;
	int textoffset, xoffset, yoffset, textdone, main_width;
	char textpart[10], addon[6];
	int large_bar_count, comp_offset;
	float addon_text_posn;
	float scaler = symbol->scale;
	float default_text_posn;
	SString temp_buf;
	const char * locale = NULL;
#ifndef _MSC_VER
	uchar local_text[sstrlen(symbol->text) + 1];
#else
	uchar* local_text = (uchar *)_alloca(sstrlen(symbol->text) + 1);
#endif
	row_height = 0;
	textdone = 0;
	main_width = symbol->width;
	sstrcpy(addon, "");
	comp_offset = 0;
	addon_text_posn = 0.0;
	if(symbol->show_hrt != 0) {
		/* Copy text from symbol */
		sstrcpy(local_text, symbol->text);
	}
	else {
		/* No text needed */
		switch(symbol->Std) {
			case BARCODE_EANX:
			case BARCODE_EANX_CC:
			case BARCODE_ISBNX:
			case BARCODE_UPCA:
			case BARCODE_UPCE:
			case BARCODE_UPCA_CC:
			case BARCODE_UPCE_CC:
			    // For these symbols use dummy text to ensure formatting is done
			    // properly even if no text is required
			    for(i = 0; i < sstrlen(symbol->text); i++) {
				    local_text[i] = (symbol->text[i] == '+') ? '+' : ' ';
				    local_text[sstrlen(symbol->text)] = '\0';
			    }
			    break;
			default:
			    /* For everything else, just remove the text */
			    local_text[0] = '\0';
			    break;
		}
	}
	fsvg = (symbol->output_options & BARCODE_STDOUT) ? stdout : fopen(symbol->outfile, "w");
	if(fsvg == NULL) {
		sstrcpy(symbol->errtxt, "Could not open output file (F60)");
		return ZINT_ERROR_FILE_ACCESS;
	}
	/* @sobolev
	// sort out colour options 
	to_upper((uchar *)symbol->fgcolour);
	to_upper((uchar *)symbol->bgcolour);
	if(strlen(symbol->fgcolour) != 6) {
		sstrcpy(symbol->errtxt, "Malformed foreground colour target (F61)");
		fclose(fsvg);
		return ZINT_ERROR_INVALID_OPTION;
	}
	if(strlen(symbol->bgcolour) != 6) {
		sstrcpy(symbol->errtxt, "Malformed background colour target (F62)");
		fclose(fsvg);
		return ZINT_ERROR_INVALID_OPTION;
	}
	error_number = is_sane(SSET, (uchar *)symbol->fgcolour, strlen(symbol->fgcolour));
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Malformed foreground colour target (F63)");
		fclose(fsvg);
		return ZINT_ERROR_INVALID_OPTION;
	}
	error_number = is_sane(SSET, (uchar *)symbol->bgcolour, strlen(symbol->bgcolour));
	if(error_number == ZINT_ERROR_INVALID_DATA) {
		sstrcpy(symbol->errtxt, "Malformed background colour target (F64)");
		fclose(fsvg);
		return ZINT_ERROR_INVALID_OPTION;
	}
	*/
	locale = setlocale(LC_ALL, "C");
	SETIFZ(symbol->height, 50);
	large_bar_count = 0;
	preset_height = 0.0;
	for(i = 0; i < (uint)symbol->rows; i++) {
		preset_height += symbol->row_height[i];
		if(symbol->row_height[i] == 0)
			large_bar_count++;
	}
	large_bar_height = (symbol->height - preset_height) / large_bar_count;
	if(large_bar_count == 0) {
		symbol->height = (int)preset_height;
	}
	while(!(module_is_set(symbol, symbol->rows - 1, comp_offset))) {
		comp_offset++;
	}
	/* Certain symbols need whitespace otherwise characters get chopped off the sides */
	if((((symbol->Std == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->Std == BARCODE_EANX_CC)) || (symbol->Std == BARCODE_ISBNX)) {
		switch(sstrlen(local_text)) {
			case 13: /* EAN 13 */
			case 16:
			case 19:
				SETIFZ(symbol->whitespace_width, 10);
			    main_width = 96 + comp_offset;
			    break;
			default:
			    main_width = 68 + comp_offset;
		}
	}
	if(((symbol->Std == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCA_CC)) {
		if(symbol->whitespace_width == 0) {
			symbol->whitespace_width = 10;
			main_width = 96 + comp_offset;
		}
	}
	if(((symbol->Std == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCE_CC)) {
		if(symbol->whitespace_width == 0) {
			symbol->whitespace_width = 10;
			main_width = 51 + comp_offset;
		}
	}
	latch = 0;
	r = 0;
	/* Isolate add-on text */
	if(is_extendable(symbol->Std)) {
		for(i = 0; i < sstrlen(local_text); i++) {
			if(latch == 1) {
				addon[r] = local_text[i];
				r++;
			}
			if(local_text[i] == '+') {
				latch = 1;
			}
		}
	}
	addon[r] = '\0';
	textoffset = (sstrlen(local_text) != 0) ? 9 : 0;
	xoffset = symbol->border_width + symbol->whitespace_width;
	yoffset = symbol->border_width;
	// Start writing the header 
	fprintf(fsvg, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
	fprintf(fsvg, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n");
	fprintf(fsvg, "   \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
	if(symbol->Std != BARCODE_MAXICODE) {
		fprintf(fsvg, "<svg width=\"%d\" height=\"%d\" version=\"1.1\"\n", fceili((symbol->width + xoffset + xoffset) * scaler),
		    fceili((symbol->height + textoffset + yoffset + yoffset) * scaler));
	}
	else {
		fprintf(fsvg, "<svg width=\"%d\" height=\"%d\" version=\"1.1\"\n", fceili((74.0f + xoffset + xoffset) * scaler), fceili((72.0f + yoffset + yoffset) * scaler));
	}
	fprintf(fsvg, "   xmlns=\"http://www.w3.org/2000/svg\">\n");
	if((sstrlen(local_text) != 0) && (symbol->show_hrt != 0)) {
		fprintf(fsvg, "   <desc>%s\n", local_text);
	}
	else {
		fprintf(fsvg, "   <desc>Zint Generated Symbol\n");
	}
	fprintf(fsvg, "   </desc>\n");
	fprintf(fsvg, "\n   <g id=\"barcode\" fill=\"%s\">\n", /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
	if(symbol->Std != BARCODE_MAXICODE) {
		fprintf(fsvg, "      <rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />\n",
		    fceili((symbol->width + xoffset + xoffset) * scaler),
		    fceili((symbol->height + textoffset + yoffset + yoffset) * scaler), /*symbol->bgcolour*/symbol->ColorBg.ToStr(temp_buf, SColor::fmtHEX).cptr());
	}
	else {
		fprintf(fsvg, "      <rect x=\"0\" y=\"0\" width=\"%d\" height=\"%d\" fill=\"%s\" />\n",
		    fceili((74.0F + xoffset + xoffset) * scaler), fceili((72.0F + yoffset + yoffset) * scaler), /*symbol->bgcolour*/symbol->ColorBg.ToStr(temp_buf, SColor::fmtHEX).cptr());
	}
	if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
		default_text_posn = (symbol->height + textoffset + symbol->border_width + symbol->border_width) * scaler;
	}
	else {
		default_text_posn = (symbol->height + textoffset + symbol->border_width) * scaler;
	}
	const char * p_rect_fmt     = "      <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" />\n";
	const char * p_text_fmt     = "      <text x=\"%.2f\" y=\"%.2f\" text-anchor=\"middle\"\n";
	const char * p_text_body_fmt = "         %s\n";
	const char * p_font_fam_fmt = "         font-family=\"Helvetica\" font-size=\"%.1f\" fill=\"%s\" >\n";
	const char * p_endtext_fmt = "      </text>\n";
	const char * p_circle_fmt   = "      <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\" />\n";
	if(symbol->Std == BARCODE_MAXICODE) {
		// Maxicode uses hexagons 
		float ax, ay, bx, by, cx, cy, dx, dy, ex, ey, fx, fy, mx, my;
		textoffset = 0;
		{
			if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
				fprintf(fsvg, p_rect_fmt, 0.0, 0.0, (74.0 + xoffset + xoffset) * scaler, symbol->border_width * scaler);
				fprintf(fsvg, p_rect_fmt, 0.0, (72.0 + symbol->border_width) * scaler, (74.0 + xoffset + xoffset) * scaler, symbol->border_width * scaler);
			}
			if(symbol->output_options & BARCODE_BOX) { // side bars 
				fprintf(fsvg, p_rect_fmt, 0.0, 0.0, symbol->border_width * scaler, (72.0 + (2 * symbol->border_width)) * scaler);
				fprintf(fsvg, p_rect_fmt, (74.0 + xoffset + xoffset - symbol->border_width) * scaler, 0.0, symbol->border_width * scaler, (72.0 + (2 * symbol->border_width)) * scaler);
			}
		}
		{
			const double _circle_y = (35.60 + yoffset) * scaler;
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 10.85 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 8.97 * scaler, /*symbol->bgcolour*/symbol->ColorBg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 7.10 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 5.22 * scaler, /*symbol->bgcolour*/symbol->ColorBg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 3.31 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			fprintf(fsvg, p_circle_fmt, (35.76 + xoffset) * scaler, _circle_y, 1.43 * scaler, /*symbol->bgcolour*/symbol->ColorBg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		}
		for(r = 0; r < symbol->rows; r++) {
			for(i = 0; i < (uint)symbol->width; i++) {
				if(module_is_set(symbol, r, i)) {
					// Dump a hexagon 
					my = r * 2.135f + 1.43f;
					ay = my + 1.0f + yoffset;
					by = my + 0.5f + yoffset;
					cy = my - 0.5f + yoffset;
					dy = my - 1.0f + yoffset;
					ey = my - 0.5f + yoffset;
					fy = my + 0.5f + yoffset;
					mx = (r & 1) ? ((2.46f * i) + 1.23f + 1.23f) : ((2.46f * i) + 1.23f);
					ax = mx + xoffset;
					bx = mx + 0.86f + xoffset;
					cx = mx + 0.86f + xoffset;
					dx = mx + xoffset;
					ex = mx - 0.86f + xoffset;
					fx = mx - 0.86f + xoffset;
					fprintf(fsvg, "      <path d=\"M %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f Z\" />\n",
					    ax * scaler, ay * scaler, bx * scaler, by * scaler, cx * scaler, cy * scaler,
					    dx * scaler, dy * scaler, ex * scaler, ey * scaler, fx * scaler, fy * scaler);
				}
			}
		}
	}
	if(symbol->Std != BARCODE_MAXICODE) {
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
			for(i = 0; i < (uint)r; i++) {
				if(symbol->row_height[i] == 0) {
					row_posn += large_bar_height;
				}
				else {
					row_posn += symbol->row_height[i];
				}
			}
			row_posn += yoffset;
			if(symbol->output_options & BARCODE_DOTTY_MODE) {
				// Use (currently undocumented) dot mode - see SF ticket #29 
				for(i = 0; i < (uint)symbol->width; i++) {
					if(module_is_set(symbol, this_row, i)) {
						fprintf(fsvg, p_circle_fmt, ((i + xoffset) * scaler) + (scaler / 2.0), (row_posn * scaler) + (scaler / 2.0),
						    (symbol->dot_size / 2.0) * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
					}
				}
			}
			else {
				/* Normal mode, with rectangles */
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
					if((addon_latch == 0) && (r == (symbol->rows - 1)) && (i > (uint)main_width)) {
						addon_text_posn = (row_posn + 8.0f) * scaler;
						addon_latch = 1;
					}
					if(latch == 1) { // a bar 
						if(addon_latch == 0) {
							fprintf(fsvg, p_rect_fmt, (i + xoffset) * scaler, row_posn * scaler, block_width * scaler, row_height * scaler);
						}
						else {
							fprintf(fsvg, p_rect_fmt, (i + xoffset) * scaler, (row_posn + 10.0) * scaler, block_width * scaler, (row_height - 5.0) * scaler);
						}
						latch = 0;
					}
					else { // a space 
						latch = 1;
					}
					i += block_width;
				} while(i < (uint)symbol->width);
			}
		}
	}
	/* That's done the actual data area, everything else is human-friendly */
	xoffset += comp_offset;
	row_posn = (row_posn + large_bar_height) * scaler;
	if((((symbol->Std == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->Std == BARCODE_EANX_CC)) || (symbol->Std == BARCODE_ISBNX)) {
		/* guard bar extensions and text formatting for EAN8 and EAN13 */
		switch(sstrlen(local_text)) {
			case 8: /* EAN-8 */
			case 11:
			case 14:
			    fprintf(fsvg, p_rect_fmt, (0 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (2 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (32 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (34 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (64 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (66 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    for(i = 0; i < 4; i++) {
				    textpart[i] = local_text[i];
			    }
			    textpart[4] = '\0';
			    textpos = 17;
			    fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, textpart);
			    fprintf(fsvg, p_endtext_fmt);
			    for(i = 0; i < 4; i++) {
				    textpart[i] = local_text[i + 4];
			    }
			    textpart[4] = '\0';
			    textpos = 50;
			    fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, textpart);
			    fprintf(fsvg, p_endtext_fmt);
			    textdone = 1;
			    switch(strlen(addon)) {
				    case 2:
					textpos = (float)(xoffset + 86);
					fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
					fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
					fprintf(fsvg, p_text_body_fmt, addon);
					fprintf(fsvg, p_endtext_fmt);
					break;
				    case 5:
					textpos = (float)(xoffset + 100);
					fprintf(fsvg, p_text_fmt, textpos * scaler,	addon_text_posn * scaler);
					fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
					fprintf(fsvg, p_text_body_fmt, addon);
					fprintf(fsvg, p_endtext_fmt);
					break;
			    }
			    break;
			case 13: /* EAN 13 */
			case 16:
			case 19:
			    fprintf(fsvg, p_rect_fmt, (0 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (2 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (46 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (48 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (92 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    fprintf(fsvg, p_rect_fmt, (94 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
			    textpart[0] = local_text[0];
			    textpart[1] = '\0';
			    textpos = -7;
			    fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, textpart);
			    fprintf(fsvg, p_endtext_fmt);
			    for(i = 0; i < 6; i++) {
				    textpart[i] = local_text[i + 1];
			    }
			    textpart[6] = '\0';
			    textpos = 24;
			    fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, textpart);
			    fprintf(fsvg, p_endtext_fmt);
			    for(i = 0; i < 6; i++) {
				    textpart[i] = local_text[i + 7];
			    }
			    textpart[6] = '\0';
			    textpos = 71;
			    fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, textpart);
			    fprintf(fsvg, p_endtext_fmt);
			    textdone = 1;
			    switch(strlen(addon)) {
				    case 2:
					textpos = (float)(xoffset + 114);
					fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
					fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
					fprintf(fsvg, p_text_body_fmt, addon);
					fprintf(fsvg, p_endtext_fmt);
					break;
				    case 5:
					textpos = (float)(xoffset + 128);
					fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
					fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
					fprintf(fsvg, p_text_body_fmt, addon);
					fprintf(fsvg, p_endtext_fmt);
					break;
			    }
			    break;
		}
	}
	if(((symbol->Std == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCA_CC)) {
		// guard bar extensions and text formatting for UPCA 
		latch = 1;
		i = 0 + comp_offset;
		do {
			block_width = 0;
			do {
				block_width++;
			} while(module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
			if(latch == 1) { // a bar 
				fprintf(fsvg, p_rect_fmt, (i + xoffset - comp_offset) * scaler, row_posn, block_width * scaler, 5.0 * scaler);
				latch = 0;
			}
			else { // a space 
				latch = 1;
			}
			i += block_width;
		} while(i < (uint)(11 + comp_offset));
		fprintf(fsvg, p_rect_fmt, (46 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		fprintf(fsvg, p_rect_fmt, (48 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		latch = 1;
		i = 85 + comp_offset;
		do {
			block_width = 0;
			do {
				block_width++;
			} while(module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
			if(latch == 1) { // a bar 
				fprintf(fsvg, p_rect_fmt, (i + xoffset - comp_offset) * scaler, row_posn, block_width * scaler, 5.0 * scaler);
				latch = 0;
			}
			else { // a space 
				latch = 1;
			}
			i += block_width;
		} while(i < (uint)(96 + comp_offset));
		textpart[0] = local_text[0];
		textpart[1] = '\0';
		textpos = -5;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 8.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		for(i = 0; i < 5; i++) {
			textpart[i] = local_text[i + 1];
		}
		textpart[5] = '\0';
		textpos = 27;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		for(i = 0; i < 5; i++) {
			textpart[i] = local_text[i + 6];
		}
		textpart[6] = '\0';
		textpos = 68;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		textpart[0] = local_text[11];
		textpart[1] = '\0';
		textpos = 100;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 8.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		textdone = 1;
		switch(strlen(addon)) {
			case 2:
			    textpos = (float)(xoffset + 116);
			    fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, addon);
			    fprintf(fsvg, p_endtext_fmt);
			    break;
			case 5:
			    textpos = (float)(xoffset + 130);
			    fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, addon);
			    fprintf(fsvg, p_endtext_fmt);
			    break;
		}
	}
	if(((symbol->Std == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->Std == BARCODE_UPCE_CC)) {
		// guard bar extensions and text formatting for UPCE 
		fprintf(fsvg, p_rect_fmt, (0 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		fprintf(fsvg, p_rect_fmt, (2 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		fprintf(fsvg, p_rect_fmt, (46 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		fprintf(fsvg, p_rect_fmt, (48 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		fprintf(fsvg, p_rect_fmt, (50 + xoffset) * scaler, row_posn, scaler, 5.0 * scaler);
		textpart[0] = local_text[0];
		textpart[1] = '\0';
		textpos = -5;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 8.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		for(i = 0; i < 6; i++) {
			textpart[i] = local_text[i + 1];
		}
		textpart[6] = '\0';
		textpos = 24;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		textpart[0] = local_text[7];
		textpart[1] = '\0';
		textpos = 55;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 8.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, textpart);
		fprintf(fsvg, p_endtext_fmt);
		textdone = 1;
		switch(strlen(addon)) {
			case 2:
			    textpos = (float)(xoffset + 70);
			    fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, addon);
			    fprintf(fsvg, p_endtext_fmt);
			    break;
			case 5:
			    textpos = (float)(xoffset + 84);
			    fprintf(fsvg, p_text_fmt, textpos * scaler, addon_text_posn * scaler);
			    fprintf(fsvg, p_font_fam_fmt, 11.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
			    fprintf(fsvg, p_text_body_fmt, addon);
			    fprintf(fsvg, p_endtext_fmt);
			    break;
		}
	}
	xoffset -= comp_offset;
	switch(symbol->Std) {
		case BARCODE_MAXICODE:
		    // Do nothing! (It's already been done) 
		    break;
		default:
		    if(symbol->output_options & BARCODE_BIND) {
			    if((symbol->rows > 1) && (is_stackable(symbol->Std) == 1)) {
				    // row binding 
				    if(symbol->Std != BARCODE_CODABLOCKF) {
					    for(r = 1; r < symbol->rows; r++) {
						    fprintf(fsvg, p_rect_fmt, xoffset * scaler, ((r * row_height) + yoffset - 1) * scaler, symbol->width * scaler, 2.0 * scaler);
					    }
				    }
				    else {
					    for(r = 1; r < symbol->rows; r++) {
						    fprintf(fsvg, p_rect_fmt, (xoffset + 11) * scaler, ((r * row_height) + yoffset - 1) * scaler, (symbol->width - 25) * scaler, 2.0 * scaler);
					    }
				    }
			    }
		    }
		    if((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
			    if(symbol->Std != BARCODE_CODABLOCKF) {
				    fprintf(fsvg, p_rect_fmt, 0.0, 0.0, (symbol->width + xoffset + xoffset) * scaler, symbol->border_width * scaler);
				    fprintf(fsvg, p_rect_fmt, 0.0, (symbol->height + symbol->border_width) * scaler, (symbol->width + xoffset + xoffset) * scaler, symbol->border_width * scaler);
			    }
			    else {
				    fprintf(fsvg, p_rect_fmt, xoffset * scaler, 0.0, symbol->width * scaler, symbol->border_width * scaler);
				    fprintf(fsvg, p_rect_fmt, xoffset * scaler, (symbol->height + symbol->border_width) * scaler, symbol->width * scaler, symbol->border_width * scaler);
			    }
		    }
		    if(symbol->output_options & BARCODE_BOX) {
			    /* side bars */
			    fprintf(fsvg, p_rect_fmt, 0.0, 0.0, symbol->border_width * scaler, (symbol->height + (2 * symbol->border_width)) * scaler);
			    fprintf(fsvg, p_rect_fmt, (symbol->width + xoffset + xoffset - symbol->border_width) * scaler, 0.0, symbol->border_width * scaler, (symbol->height + (2 * symbol->border_width)) * scaler);
		    }
		    break;
	}
	// Put the human readable text at the bottom 
	if(!textdone && sstrlen(local_text)) {
		textpos = symbol->width / 2.0f;
		fprintf(fsvg, p_text_fmt, (textpos + xoffset) * scaler, default_text_posn);
		fprintf(fsvg, p_font_fam_fmt, 8.0 * scaler, /*symbol->fgcolour*/symbol->ColorFg.ToStr(temp_buf, SColor::fmtHEX).cptr());
		fprintf(fsvg, p_text_body_fmt, local_text);
		fprintf(fsvg, p_endtext_fmt);
	}
	fprintf(fsvg, "   </g>\n");
	fprintf(fsvg, "</svg>\n");
	if(symbol->output_options & BARCODE_STDOUT) {
		fflush(fsvg);
	}
	else {
		fclose(fsvg);
	}
	if(locale)
		setlocale(LC_ALL, locale);
	return error_number;
}
