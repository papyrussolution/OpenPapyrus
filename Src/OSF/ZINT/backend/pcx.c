/* pcx.c - Handles output to ZSoft PCX file */

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

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */
#include "common.h"
#pragma hdrstop

#define SSET    "0123456789ABCDEF"
//
// PCX header structure 
//
#pragma pack (1)
    
	struct pcx_header_t {
		uint8  manufacturer;
		uint8  version;
		uint8  encoding;
		uint8  bits_per_pixel;
		uint16 window_xmin;
		uint16 window_ymin;
		uint16 window_xmax;
		uint16 window_ymax;
		uint16 horiz_dpi;
		uint16 vert_dpi;
		uint8  colourmap[48];
		uint8  reserved;
		uint8  number_of_planes;
		uint16 bytes_per_line;
		uint16 palette_info;
		uint16 horiz_screen_size;
		uint16 vert_screen_size;
		uint8  filler[54];
	};

#pragma pack ()

int pcx_pixel_plot(struct ZintSymbol * symbol, char * pixelbuf)
{
	int row, column, i, colour;
	int run_count;
	FILE * pcx_file;
	pcx_header_t header;
#ifdef _MSC_VER
	uchar* rle_row;
#endif
#ifndef _MSC_VER
	uchar rle_row[symbol->bitmap_width];
#else
	rle_row = (uchar *)_alloca((symbol->bitmap_width * 6) * sizeof(uchar));
#endif /* _MSC_VER */
	//const int fgred = (16 * hex(symbol->fgcolour[0])) + hex(symbol->fgcolour[1]);
	//const int fggrn = (16 * hex(symbol->fgcolour[2])) + hex(symbol->fgcolour[3]);
	//const int fgblu = (16 * hex(symbol->fgcolour[4])) + hex(symbol->fgcolour[5]);
	//const int bgred = (16 * hex(symbol->bgcolour[0])) + hex(symbol->bgcolour[1]);
	//const int bggrn = (16 * hex(symbol->bgcolour[2])) + hex(symbol->bgcolour[3]);
	//const int bgblu = (16 * hex(symbol->bgcolour[4])) + hex(symbol->bgcolour[5]);
	const int fgred = symbol->ColorFg.R;
	const int fggrn = symbol->ColorFg.G;
	const int fgblu = symbol->ColorFg.B;
	const int bgred = symbol->ColorBg.R;
	const int bggrn = symbol->ColorBg.G;
	const int bgblu = symbol->ColorBg.B;

	header.manufacturer = 10; // ZSoft
	header.version = 5; // Version 3.0
	header.encoding = 1; // Run length encoding
	header.bits_per_pixel = 8;
	header.window_xmin = 0;
	header.window_ymin = 0;
	header.window_xmax = symbol->bitmap_width - 1;
	header.window_ymax = symbol->bitmap_height - 1;
	header.horiz_dpi = 300;
	header.vert_dpi = 300;
	for(i = 0; i < 48; i++) {
		header.colourmap[i] = 0x00;
	}
	header.reserved = 0;
	header.number_of_planes = 3;

	if(symbol->bitmap_width % 2) {
		header.bytes_per_line = symbol->bitmap_width + 1;
	}
	else {
		header.bytes_per_line = symbol->bitmap_width;
	}

	header.palette_info = 1; // Colour
	header.horiz_screen_size = 0;
	header.vert_screen_size = 0;

	for(i = 0; i < 54; i++) {
		header.filler[i] = 0x00;
	}

	/* Open output file in binary mode */
	if(symbol->output_options & BARCODE_STDOUT) {
#ifdef _MSC_VER
		if(-1 == _setmode(_fileno(stdout), _O_BINARY)) {
			sstrcpy(symbol->errtxt, "Can't open output file");
			return ZINT_ERROR_FILE_ACCESS;
		}
#endif
		pcx_file = stdout;
	}
	else {
		if(!(pcx_file = fopen(symbol->outfile, "wb"))) {
			sstrcpy(symbol->errtxt, "Can't open output file (F20)");
			return ZINT_ERROR_FILE_ACCESS;
		}
	}

	fwrite(&header, sizeof(pcx_header_t), 1, pcx_file);

	for(row = 0; row < symbol->bitmap_height; row++) {
		for(colour = 0; colour < 3; colour++) {
			for(column = 0; column < symbol->bitmap_width; column++) {
				switch(colour) {
					case 0:
					    if(pixelbuf[(row * symbol->bitmap_width) + column] == '1') {
						    rle_row[column] = fgred;
					    }
					    else {
						    rle_row[column] = bgred;
					    }
					    break;
					case 1:
					    if(pixelbuf[(row * symbol->bitmap_width) + column] == '1') {
						    rle_row[column] = fggrn;
					    }
					    else {
						    rle_row[column] = bggrn;
					    }
					    break;
					case 2:
					    if(pixelbuf[(row * symbol->bitmap_width) + column] == '1') {
						    rle_row[column] = fgblu;
					    }
					    else {
						    rle_row[column] = bgblu;
					    }
					    break;
				}
			}
			run_count = 1;
			for(column = 1; column < symbol->bitmap_width; column++) {
				if((rle_row[column - 1] == rle_row[column]) && (run_count < 63)) {
					run_count++;
				}
				else {
					run_count += 0xc0;
					fputc(run_count, pcx_file);
					fputc(rle_row[column - 1], pcx_file);
					run_count = 1;
				}
			}
			if(run_count > 1) {
				run_count += 0xc0;
				fputc(run_count, pcx_file);
				fputc(rle_row[column - 1], pcx_file);
			}
		}
	}
	fclose(pcx_file);
	return 0;
}