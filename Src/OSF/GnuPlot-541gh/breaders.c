// GNUPLOT - breaders.c 
// Copyright 2004  Petr Mikulik
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
// AUTHOR : Petr Mikulik 
/*
 * Readers to set up binary data file information for particular formats.
 */
#include <gnuplot.h>
#pragma hdrstop
//
// Reader for the ESRF Header File format files (EDF / EHF).
//
// gen_table4 
struct gen_table4 {
	const char * key;
	int    value;
	short  signum; /* 0..unsigned, 1..signed, 2..float or double */
	short  sajzof; /* sizeof on 32bit architecture */
};
// 
// Exactly like lookup_table_nth from tables.c, but for gen_table4 instead
// of gen_table.
// 
static int lookup_table4_nth(const struct gen_table4 * tbl, const char * search_str)
{
	for(int k = -1; tbl[++k].key;)
		if(tbl[k].key && !strncmp(search_str, tbl[k].key, strlen(tbl[k].key)))
			return k;
	return -1; // not found 
}

static const struct gen_table4 edf_datatype_table[] = {
	{ "UnsignedByte",   DF_UCHAR,   0, 1 },
	{ "SignedByte",     DF_CHAR,    1, 1 },
	{ "UnsignedShort",  DF_USHORT,  0, 2 },
	{ "SignedShort",    DF_SHORT,   1, 2 },
	{ "UnsignedInteger", DF_UINT,    0, 4 },
	{ "SignedInteger",  DF_INT,     1, 4 },
	{ "UnsignedLong",   DF_ULONG,   0, 8 },
	{ "SignedLong",     DF_LONG,    1, 8 },
	{ "FloatValue",     DF_FLOAT,   2, 4 },
	{ "DoubleValue",    DF_DOUBLE,  2, 8 },
	{ "Float",          DF_FLOAT,   2, 4 },/* Float and FloatValue are synonyms */
	{ "Double",         DF_DOUBLE,  2, 8 },/* Double and DoubleValue are synonyms */
	{ NULL, -1, -1, -1 }
};

static const struct gen_table edf_byteorder_table[] = {
	{ "LowByteFirst",   DF_LITTLE_ENDIAN },/* little endian */
	{ "HighByteFirst",  DF_BIG_ENDIAN },/* big endian */
	{ NULL, -1 }
};

/* Orientation of axes of the raster, as the binary matrix is saved in
 * the file.
 */
enum EdfRasterAxes {
	EDF_RASTER_AXES_XrightYdown,    /* matricial format: rows, columns */
	EDF_RASTER_AXES_XrightYup       /* cartesian coordinate system */
	/* other 6 combinations not available (not needed until now) */
};

static const struct gen_table edf_rasteraxes_table[] = {
	{ "XrightYdown",    EDF_RASTER_AXES_XrightYdown },
	{ "XrightYup",      EDF_RASTER_AXES_XrightYup },
	{ NULL, -1 }
};
// 
// Find value_ptr as pointer to the parameter of the given key in the header.
// Returns NULL on success.
//
static const char * FASTCALL edf_findInHeader(const char * header, const char * key)
{
	const char * value_ptr = strstr(header, key);
	if(value_ptr) {
		// an edf line is "key     = value ;" 
		value_ptr = 1 + sstrchr(value_ptr + strlen(key), '=');
		while(isspace((uchar)*value_ptr))
			value_ptr++;
	}
	return value_ptr;
}

//void edf_filetype_function()
void GnuPlot::FileTypeFunction_Edf()
{
	char * header = NULL;
	int header_size = 0;
	const char * p;
	int k;
	FILE * fp = LoadPath_fopen(_Df.df_filename, "rb"); // open (header) file 
	if(!fp)
		OsError(NO_CARET, "Can't open data file \"%s\"", _Df.df_filename);
	// read header: it is a multiple of 512 B ending by "}\n" 
	while(!header_size || strncmp(&header[header_size-2], "}\n", 2)) {
		int header_size_prev = header_size;
		header_size += 512;
		header = (char *)SAlloc::R(header, header_size+1);
		header[header_size_prev] = 0; // protection against empty file 
		k = fread(header+header_size_prev, 512, 1, fp);
		if(k == 0) { // protection against indefinite loop 
			SAlloc::F(header);
			OsError(NO_CARET, "Damaged EDF header of %s: not multiple of 512 B.\n", _Df.df_filename);
		}
		header[header_size] = 0; // end of string: protection against strstr later on 
	}
	fclose(fp);
	// make sure there is a binary record structure for each image 
	if(_Df.NumBinRecords < 1)
		DfAddBinaryRecords(1-_Df.NumBinRecords, DF_CURRENT_RECORDS); // otherwise put here: number of images (records) from this file 
	p = edf_findInHeader(header, "EDF_BinaryFileName");
	if(p) {
		int plen = strcspn(p, " ;\n");
		_Df.df_filename = (char *)SAlloc::R(_Df.df_filename, plen+1);
		strnzcpy(_Df.df_filename, p, plen+1);
		p = edf_findInHeader(header, "EDF_BinaryFilePosition");
		_Df.df_bin_record[0].scan_skip[0] = satoi(p);
	}
	else
		_Df.df_bin_record[0].scan_skip[0] = header_size; // skip header 
	// set default values 
	_Df.df_bin_record[0].scan_dir[0] = 1;
	_Df.df_bin_record[0].scan_dir[1] = -1;
	_Df.df_bin_record[0].scan_generate_coord = TRUE;
	_Df.df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
	_Df.df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;
	DfExtendBinaryColumns(1);
	DfSetSkipBefore(1, 0);
	df_set_skip_after(1, 0);
	_Df.df_no_use_specs = 1;
	_Df.UseSpec[0].column = 1;
	// now parse the header 
	p = edf_findInHeader(header, "Dim_1");
	if(p)
		_Df.df_bin_record[0].scan_dim[0] = satoi(p);
	p = edf_findInHeader(header, "Dim_2");
	if(p)
		_Df.df_bin_record[0].scan_dim[1] = satoi(p);
	p = edf_findInHeader(header, "DataType");
	if(p) {
		k = lookup_table4_nth(edf_datatype_table, p);
		if(k >= 0) { /* known EDF DataType */
			int s = edf_datatype_table[k].sajzof;
			switch(edf_datatype_table[k].signum) {
				case 0: DfSetReadType(1, SIGNED_TEST(s)); break;
				case 1: DfSetReadType(1, UNSIGNED_TEST(s)); break;
				case 2: DfSetReadType(1, FLOAT_TEST(s)); break;
			}
		}
	}
	p = edf_findInHeader(header, "ByteOrder");
	if(p) {
		k = lookup_table_nth(edf_byteorder_table, p);
		if(k >= 0)
			_Df.BinFileEndianess = (df_endianess_type)edf_byteorder_table[k].value;
	}
	// Origin vs center: EDF specs allows only Center, but it does not hurt if
	// Origin is supported as well; however, Center rules if both specified.
	p = edf_findInHeader(header, "Origin_1");
	if(p) {
		_Df.df_bin_record[0].scan_cen_or_ori[0] = satof(p);
		_Df.df_bin_record[0].scan_trans = DF_TRANSLATE_VIA_ORIGIN;
	}
	p = edf_findInHeader(header, "Origin_2");
	if(p) {
		_Df.df_bin_record[0].scan_cen_or_ori[1] = satof(p);
		_Df.df_bin_record[0].scan_trans = DF_TRANSLATE_VIA_ORIGIN;
	}
	p = edf_findInHeader(header, "Center_1");
	if(p) {
		_Df.df_bin_record[0].scan_cen_or_ori[0] = satof(p);
		_Df.df_bin_record[0].scan_trans = DF_TRANSLATE_VIA_CENTER;
	}
	p = edf_findInHeader(header, "Center_2");
	if(p) {
		_Df.df_bin_record[0].scan_cen_or_ori[1] = satof(p);
		_Df.df_bin_record[0].scan_trans = DF_TRANSLATE_VIA_CENTER;
	}
	// now pixel sizes and raster orientation 
	p = edf_findInHeader(header, "PSize_1");
	if(p)
		_Df.df_bin_record[0].scan_delta[0] = satof(p);
	p = edf_findInHeader(header, "PSize_2");
	if(p)
		_Df.df_bin_record[0].scan_delta[1] = satof(p);
	p = edf_findInHeader(header, "RasterAxes");
	if(p) {
		k = lookup_table_nth(edf_rasteraxes_table, p);
		switch(k) {
			case EDF_RASTER_AXES_XrightYup:
			    _Df.df_bin_record[0].scan_dir[0] = 1;
			    _Df.df_bin_record[0].scan_dir[1] = 1;
			    _Df.df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
			    _Df.df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;
			    break;
			default: // also EDF_RASTER_AXES_XrightYdown 
			    _Df.df_bin_record[0].scan_dir[0] = 1;
			    _Df.df_bin_record[0].scan_dir[1] = -1;
			    _Df.df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
			    _Df.df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;
		}
	}
	SAlloc::F(header);
}
/*
 *	Use libgd for input of binary images in PNG GIF JPEG formats
 *	Ethan A Merritt - August 2009
 */
#define GD_PNG 1
#define GD_GIF 2
#define GD_JPEG 3
//void gd_filetype_function(int filetype);
//void png_filetype_function() { gd_filetype_function(GD_PNG); }
//void gif_filetype_function() { gd_filetype_function(GD_GIF); }
//void jpeg_filetype_function() { gd_filetype_function(GD_JPEG); }

void GnuPlot::FileTypeFunction_Png() { Implement_FileTypeFunction_Gd(GD_PNG); }
void GnuPlot::FileTypeFunction_Gif() { Implement_FileTypeFunction_Gd(GD_GIF); }
void GnuPlot::FileTypeFunction_Jpeg() { Implement_FileTypeFunction_Gd(GD_JPEG); }

#ifndef HAVE_GD_PNG

//void gd_filetype_function(int type)
void GnuPlot::Implement_FileTypeFunction_Gd(int type)
{
	IntError(NO_CARET, "This copy of gnuplot cannot read png/gif/jpeg images");
}

int df_libgd_get_pixel(int i, int j, int component) { return 0; }

//bool df_read_pixmap(t_pixmap * pixmap)
bool GnuPlot::DfReadPixmap(t_pixmap * pixmap)
{
	IntWarn(NO_CARET, "This copy of gnuplot cannot read png/gif/jpeg images");
	return FALSE;
}

#else

#include <gd.h>
static gdImagePtr im = NULL;

//void gd_filetype_function(int filetype)
void GnuPlot::Implement_FileTypeFunction_Gd(int filetype)
{
	FILE * fp;
	uint M, N;
	// free previous image, if any 
	if(im) {
		gdImageDestroy(im);
		im = NULL;
	}
	// read image into memory 
	fp = LoadPath_fopen(df_filename, "rb");
	if(!fp)
		IntError(NO_CARET, "Can't open data file \"%s\"", df_filename);
	switch(filetype) {
		case GD_PNG:    im = gdImageCreateFromPng(fp); break;
		case GD_GIF:
#ifdef HAVE_GD_GIF
		    im = gdImageCreateFromGif(fp);
#endif
		    break;
		case GD_JPEG:
#ifdef HAVE_GD_JPEG
		    im = gdImageCreateFromJpeg(fp);
#endif
		default:        break;
	}
	fclose(fp);
	if(!im)
		IntError(NO_CARET, "libgd doesn't recognize the format of \"%s\"", df_filename);
	/* check on image properties and complain if we can't handle them */
	M = im->sx;
	N = im->sy;
	FPRINTF((stderr, "This is a %u X %u %s image\n", M, N, im->trueColor ? "TrueColor" : "palette"));
	df_pixeldata = im->trueColor ? (void *)im->tpixels : (void *)im->pixels;
	df_matrix_file = FALSE;
	df_binary_file = TRUE;
	df_bin_record[0].scan_skip[0] = 0;
	df_bin_record[0].scan_dim[0] = M;
	df_bin_record[0].scan_dim[1] = N;
	df_bin_record[0].scan_dir[0] = 1;
	df_bin_record[0].scan_dir[1] = -1;
	df_bin_record[0].scan_generate_coord = TRUE;
	df_bin_record[0].cart_scan[0] = DF_SCAN_POINT;
	df_bin_record[0].cart_scan[1] = DF_SCAN_LINE;

	DfExtendBinaryColumns(4);
	DfSetReadType(1, DF_UCHAR);
	DfSetReadType(2, DF_UCHAR);
	DfSetReadType(3, DF_UCHAR);
	DfSetReadType(4, DF_UCHAR);
	DfSetSkipBefore(1, 0);

	df_no_use_specs = 4;
	_Df.UseSpec[0].column = 1;
	_Df.UseSpec[1].column = 2;
	_Df.UseSpec[2].column = 3;
	_Df.UseSpec[3].column = 4;
}

int df_libgd_get_pixel(int i, int j, int component)
{
	static int pixel;
	int alpha;
	switch(component) {
		case 0:     pixel = gdImageGetTrueColorPixel(im, i, j);
		    return gdTrueColorGetRed(pixel);
		case 1: return gdTrueColorGetGreen(pixel);
		case 2: return gdTrueColorGetBlue(pixel);
		case 3: /* runs from 0-127 rather than 0-255 */
		    alpha = 2 * gdTrueColorGetAlpha(pixel);
		    return (255-alpha);
		default: return 0; /* shouldn't happen */
	}
}

//bool df_read_pixmap(t_pixmap * pixmap)
bool GnuPlot::DfReadPixmap(t_pixmap * pixmap)
{
	int filetype;
	int i, j;
	coordval * pixel;
	char * file_ext = sstrrchr(pixmap->filename, '.');
	// Parse file name 
	if(!file_ext++)
		return FALSE;
	if(sstreqi_ascii(file_ext, "png"))
		filetype = GD_PNG;
	else if(sstreqi_ascii(file_ext, "gif"))
		filetype = GD_GIF;
	else if(sstreqi_ascii(file_ext, "jpeg") || sstreqi_ascii(file_ext, "jpg"))
		filetype = GD_JPEG;
	else {
		// Clear anything that was there before 
		pixmap->nrows = pixmap->ncols = 0;
		IntWarn(NO_CARET, "unrecognized pixmap type: %s", pixmap->filename);
		return FALSE;
	}
	// Create a blank record that gd_filetype_function can write into 
	DfAddBinaryRecords(1, DF_CURRENT_RECORDS);
	// Open file and allocate space for image data 
	df_filename = (char *)pixmap->filename;
	Implement_FileTypeFunction_Gd(filetype);
	df_filename = NULL;
	pixmap->ncols = df_bin_record[0].scan_dim[0];
	pixmap->nrows = df_bin_record[0].scan_dim[1];
	pixmap->image_data = SAlloc::R(pixmap->image_data, 4.0 * sizeof(coordval) * pixmap->ncols * pixmap->nrows);
	// Fill in image data 
	pixel = pixmap->image_data;
	for(i = 0; i<pixmap->nrows; i++)
		for(j = 0; j<pixmap->ncols; j++) {
			*pixel++ = (coordval)df_libgd_get_pixel(j, i, 0) / 255.0;
			*pixel++ = (coordval)df_libgd_get_pixel(j, i, 1) / 255.0;
			*pixel++ = (coordval)df_libgd_get_pixel(j, i, 2) / 255.0;
			*pixel++ = (coordval)df_libgd_get_pixel(j, i, 3);
		}
	return TRUE;
}

#endif
