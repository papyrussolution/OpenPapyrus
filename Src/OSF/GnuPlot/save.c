/* GNUPLOT - save.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
   ]*/
#include <gnuplot.h>
#pragma hdrstop

const char * coord_msg[] = {"first ", "second ", "graph ", "screen ", "character "};
/*
 *  functions corresponding to the arguments of the GNUPLOT `save` command
 */

//void save_functions(FILE * fp)
void GpGadgets::SaveFunctions(GpCommand & rC, FILE * fp)
{
	// I _love_ information written at the top and the end
	// of a human readable ASCII file
	show_version(rC, fp);
	Ev.SaveFunctions__sub(fp);
	fputs("#    EOF\n", fp);
}

//void save_variables(FILE * fp)
void GpGadgets::SaveVariables(GpCommand & rC, FILE * fp)
{
	show_version(rC, fp);
	Ev.SaveVariables__sub(fp);
	fputs("#    EOF\n", fp);
}

//void save_set(GpCommand & rC, FILE * fp)
void GpGadgets::SaveSet(GpCommand & rC, FILE * fp)
{
	show_version(rC, fp);
	SaveSetAll(rC, fp);
	fputs("#    EOF\n", fp);
}

//void save_all(GpCommand & rC, FILE * fp)
void GpGadgets::SaveAll(GpCommand & rC, FILE * fp)
{
	show_version(rC, fp);
	SaveSetAll(rC, fp);
	Ev.SaveFunctions__sub(fp);
	Ev.SaveVariables__sub(fp);
	if(GpDf.df_filename)
		fprintf(fp, "## Last datafile plotted: \"%s\"\n", GpDf.df_filename);
	fprintf(fp, "%s\n", rC.P_ReplotLine);
	if(GpF.WriToFilLastFitCmd(NULL)) {
		fputs("## ", fp);
		GpF.WriToFilLastFitCmd(fp);
		putc('\n', fp);
	}
	fputs("#    EOF\n", fp);
}
//
// auxiliary functions
//
//static void save_functions__sub(FILE * fp)
void GpEval::SaveFunctions__sub(FILE * fp)
{
	UdftEntry * p_udf = first_udf;
	while(p_udf) {
		if(p_udf->definition) {
			fprintf(fp, "%s\n", p_udf->definition);
		}
		p_udf = p_udf->next_udf;
	}
}

//static void save_variables__sub(FILE * fp)
void GpEval::SaveVariables__sub(FILE * fp)
{
	// always skip pi
	UdvtEntry * p_udv = first_udv->next_udv;
	while(p_udv) {
		if(p_udv->udv_value.type != NOTDEFINED) {
			if(p_udv->udv_value.type == ARRAY) {
				fprintf(fp, "array %s[%d] = ", p_udv->udv_name, p_udv->udv_value.v.value_array[0].v.int_val);
				save_array_content(fp, p_udv->udv_value.v.value_array);
			}
			else if(strncmp(p_udv->udv_name, "GPVAL_", 6)
			    && strncmp(p_udv->udv_name, "MOUSE_", 6)
			    && strncmp(p_udv->udv_name, "$", 1)
			    && (strncmp(p_udv->udv_name, "ARG", 3) || (strlen(p_udv->udv_name) != 4))
			    && strncmp(p_udv->udv_name, "NaN", 4)) {
				fprintf(fp, "%s = ", p_udv->udv_name);
				disp_value(fp, &(p_udv->udv_value), true);
				putc('\n', fp);
			}
		}
		p_udv = p_udv->next_udv;
	}
}

void save_array_content(FILE * fp, t_value * array)
{
	int i;
	int size = array[0].v.int_val;
	fprintf(fp, "[");
	for(i = 1; i<=size; i++) {
		if(array[i].type != NOTDEFINED)
			disp_value(fp, &(array[i]), true);
		if(i < size)
			fprintf(fp, ",");
	}
	fprintf(fp, "]\n");
}

/* HBB 19990823: new function 'save term'. This will be mainly useful
 * for the typical 'set term post ... plot ... set term <normal term>
 * sequence. It's the only 'save' function that will write the
 * current term setting to a file uncommentedly. */
void save_term(FILE * fp)
{
	show_version(GpC, fp);
	/* A possible gotcha: the default initialization often doesn't set
	 * term_options, but a 'set term <type>' without options doesn't
	 * reset the options to startup defaults. This may have to be
	 * changed on a per-terminal driver basis... */
	if(term)
		fprintf(fp, "set terminal %s %s\n", term->name, term_options);
	else
		fputs("set terminal unknown\n", fp);
	// output will still be written in commented form.  Otherwise, the
	// risk of overwriting files is just too high 
	if(outstr)
		fprintf(fp, "# set output '%s'\n", outstr);
	else
		fputs("# set output\n", fp);
	fputs("#    EOF\n", fp);
}

static void save_justification(int just, FILE * fp)
{
	switch(just) {
		case RIGHT: fputs(" right", fp); break;
		case LEFT: fputs(" left", fp); break;
		case CENTRE: fputs(" center", fp); break;
	}
}

//static void save_set_all(FILE * fp)
void GpGadgets::SaveSetAll(GpCommand & rC, FILE * fp)
{
	GpTextLabel * p_label;
	arrow_def * p_arrow;
	linestyle_def * p_line_style;
	arrowstyle_def * p_arrow_style;
	legend_key * key = &keyT;
	int axis;
	// opinions are split as to whether we save term and outfile
	// as a compromise, we output them as comments !
	if(term)
		fprintf(fp, "# set terminal %s %s\n", term->name, term_options);
	else
		fputs("# set terminal unknown\n", fp);
	if(outstr)
		fprintf(fp, "# set output '%s'\n", outstr);
	else
		fputs("# set output\n", fp);
	fprintf(fp, "%sset clip points\n%sset clip one\n%sset clip two\n", (ClipPoints) ? "" : "un",
	    (ClipLines1) ? "" : "un", (ClipLines2) ? "" : "un");
	SaveBars(fp);
	if(DrawBorder) {
		fprintf(fp, "set border %d %s", DrawBorder, BorderLayer == LAYER_BEHIND ? "behind" : 
			BorderLayer == LAYER_BACK ? "back" : "front");
		save_linetype(fp, &BorderLp, false);
		fprintf(fp, "\n");
	}
	else
		fputs("unset border\n", fp);
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(!oneof3(axis, SAMPLE_AXIS, COLOR_AXIS, POLAR_AXIS)) {
			fprintf(fp, "set %sdata %s\n", GetAxisName(axis), AxA[axis].datatype == DT_TIMEDATE ? "time" : AxA[axis].datatype == DT_DMS ? "geographic" : "");
		}
	}
	if(boxwidth < 0.0)
		fputs("set boxwidth\n", fp);
	else
		fprintf(fp, "set boxwidth %g %s\n", boxwidth, (boxwidth_is_absolute) ? "absolute" : "relative");
	fprintf(fp, "set style fill ");
	save_fillstyle(fp, &DefaultFillStyle);
#ifdef EAM_OBJECTS
	/* Default rectangle style */
	fprintf(fp, "set style rectangle %s fc ", DefaultRectangle.layer > 0 ? "front" : DefaultRectangle.layer < 0 ? "behind" : "back");
	/* FIXME: broke with removal of use_palette? */
	save_pm3dcolor(fp, &DefaultRectangle.lp_properties.pm3d_color);
	fprintf(fp, " fillstyle ");
	save_fillstyle(fp, &DefaultRectangle.fillstyle);
	/* Default circle properties */
	fprintf(fp, "set style circle radius ");
	save_position(fp, &DefaultCircle.o.circle.extent, 1, false);
	fputs(" \n", fp);
	// Default ellipse properties 
	fprintf(fp, "set style ellipse size ");
	save_position(fp, &DefaultCircle.o.ellipse.extent, 2, false);
	fprintf(fp, " angle %g ", DefaultEllipse.o.ellipse.orientation);
	fputs("units ", fp);
	switch(DefaultEllipse.o.ellipse.type) {
		case ELLIPSEAXES_XY: fputs("xy\n", fp); break;
		case ELLIPSEAXES_XX: fputs("xx\n", fp); break;
		case ELLIPSEAXES_YY: fputs("yy\n", fp); break;
	}
#endif
	if(dgrid3d) {
		if(dgrid3d_mode == DGRID3D_QNORM) {
			fprintf(fp, "set dgrid3d %d,%d, %d\n", dgrid3d_row_fineness, dgrid3d_col_fineness, dgrid3d_norm_value);
		}
		else if(dgrid3d_mode == DGRID3D_SPLINES) {
			fprintf(fp, "set dgrid3d %d,%d splines\n", dgrid3d_row_fineness, dgrid3d_col_fineness);
		}
		else {
			fprintf(fp, "set dgrid3d %d,%d %s%s %f,%f\n",
			    dgrid3d_row_fineness, dgrid3d_col_fineness,
			    reverse_table_lookup(dgrid3d_mode_tbl, dgrid3d_mode),
			    dgrid3d_kdensity ? " kdensity2d" : "", dgrid3d_x_scale, dgrid3d_y_scale);
		}
	}

	/* Dummy variable names */
	fprintf(fp, "set dummy %s", rC.P.SetDummyVar[0]);
	for(axis = 1; axis<MAX_NUM_VAR; axis++) {
		if(*rC.P.SetDummyVar[axis] == '\0')
			break;
		fprintf(fp, ", %s", rC.P.SetDummyVar[axis]);
	}
	fprintf(fp, "\n");
#define SAVE_FORMAT(axis) fprintf(fp, "set format %s \"%s\" %s\n", GetAxisName(axis), conv_text(AxA[axis].formatstring),\
	AxA[axis].tictype == DT_DMS ? "geographic" : AxA[axis].tictype == DT_TIMEDATE ? "timedate" : "");
	SAVE_FORMAT(FIRST_X_AXIS);
	SAVE_FORMAT(FIRST_Y_AXIS);
	SAVE_FORMAT(SECOND_X_AXIS);
	SAVE_FORMAT(SECOND_Y_AXIS);
	SAVE_FORMAT(FIRST_Z_AXIS);
	SAVE_FORMAT(COLOR_AXIS);
	SAVE_FORMAT(POLAR_AXIS);
#undef SAVE_FORMAT
	fprintf(fp, "set P_TimeFormat \"%s\"\n", P_TimeFormat);
	fprintf(fp, "set angles %s\n", (Ang2Rad == 1.0) ? "radians" : "degrees");
	fprintf(fp, "set tics %s\n", grid_tics_in_front ? "front" : "back");
	if(!SomeGridSelected())
		fputs("unset grid\n", fp);
	else {
		if(polar_grid_angle) // set angle already output 
			fprintf(fp, "set grid polar %f\n", polar_grid_angle / Ang2Rad);
		else
			fputs("set grid nopolar\n", fp);

#define SAVE_GRID(axis)					\
	fprintf(fp, " %s%stics %sm%stics", (AxA[axis].Flags & GpAxis::fGridMajor) ? "" : "no", GetAxisName(axis),\
		(AxA[axis].Flags & GpAxis::fGridMinor) ? "" : "no", GetAxisName(axis));
		fputs("set grid", fp);
		SAVE_GRID(FIRST_X_AXIS);
		SAVE_GRID(FIRST_Y_AXIS);
		SAVE_GRID(FIRST_Z_AXIS);
		fputs(" \\\n", fp);
		SAVE_GRID(SECOND_X_AXIS);
		SAVE_GRID(SECOND_Y_AXIS);
		SAVE_GRID(COLOR_AXIS);
		fputs("\n", fp);
#undef SAVE_GRID

		fprintf(fp, "set grid %s  ", (grid_layer==-1) ? "layerdefault" : ((grid_layer==0) ? "back" : "front"));
		save_linetype(fp, &grid_lp, false);
		fprintf(fp, ", ");
		save_linetype(fp, &mgrid_lp, false);
		fputc('\n', fp);
	}
	fprintf(fp, "%sset raxis\n", raxis ? "" : "un");
	save_style_parallel(fp); // Save parallel axis state 
	fprintf(fp, "set key title \"%s\"", conv_text(key->title.text));
	if(key->title.font)
		fprintf(fp, " font \"%s\" ", key->title.font);
	save_justification(key->title.pos, fp);
	fputs("\n", fp);

	fputs("set key ", fp);
	switch(key->region) {
		case GPKEY_AUTO_INTERIOR_LRTBC:
		    fputs("inside", fp);
		    break;
		case GPKEY_AUTO_EXTERIOR_LRTBC:
		    fputs("outside", fp);
		    break;
		case GPKEY_AUTO_EXTERIOR_MARGIN:
		    switch(key->margin) {
			    case GPKEY_TMARGIN: fputs("TMrg", fp); break;
			    case GPKEY_BMARGIN: fputs("BMrg", fp); break;
			    case GPKEY_LMARGIN: fputs("LMrg", fp); break;
			    case GPKEY_RMARGIN: fputs("RMrg", fp); break;
		    }
		    break;
		case GPKEY_USER_PLACEMENT:
		    fputs("at ", fp);
		    save_position(fp, &key->user_pos, 2, false);
		    break;
	}
	if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && oneof2(key->margin, GPKEY_LMARGIN, GPKEY_RMARGIN))) {
		save_justification(key->hpos, fp);
	}
	if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && oneof2(key->margin, GPKEY_TMARGIN, GPKEY_BMARGIN))) {
		switch(key->vpos) {
			case JUST_TOP: fputs(" top", fp); break;
			case JUST_BOT: fputs(" bottom", fp); break;
			case JUST_CENTRE: fputs(" center", fp); break;
		}
	}
	fprintf(fp, " %s %s %sreverse %senhanced %s ",
	    key->stack_dir == GPKEY_VERTICAL ? "vertical" : "horizontal",
	    key->just == GPKEY_LEFT ? "Left" : "Right",
	    key->reverse ? "" : "no",
	    key->enhanced ? "" : "no",
	    key->auto_titles == COLUMNHEAD_KEYTITLES ? "autotitle columnhead"
	    : key->auto_titles == FILENAME_KEYTITLES ? "autotitle"
	    : "noautotitle");
	if(key->box.l_type > LT_NODRAW) {
		fputs("box", fp);
		save_linetype(fp, &(key->box), false);
	}
	else
		fputs("nobox", fp);

	/* These are for the key entries, not the key title */
	if(key->font)
		fprintf(fp, " font \"%s\"", key->font);
	if(key->textcolor.type != TC_LT || key->textcolor.lt != LT_BLACK)
		save_textcolor(fp, &key->textcolor);

	/* Put less common options on separate lines */
	fprintf(fp, "\nset key %sinvert samplen %g spacing %g width %g height %g ",
	    key->invert ? "" : "no",
	    key->swidth, key->vert_factor, key->width_fix, key->height_fix);
	fprintf(fp, "\nset key maxcolumns %d maxrows %d", key->maxcols, key->maxrows);
	fputc('\n', fp);
	fprintf(fp, "set key %sopaque\n", key->front ? "" : "no");

	if(!(key->visible))
		fputs("unset key\n", fp);

	fputs("unset label\n", fp);
	for(p_label = first_label; p_label != NULL;
	    p_label = p_label->next) {
		fprintf(fp, "set label %d \"%s\" at ",
		    p_label->tag,
		    conv_text(p_label->text));
		save_position(fp, &p_label->place, 3, false);
		if(p_label->hypertext)
			fprintf(fp, " hypertext");

		save_justification(p_label->pos, fp);
		if(p_label->rotate)
			fprintf(fp, " rotate by %d", p_label->rotate);
		else
			fprintf(fp, " norotate");
		if(p_label->font != NULL)
			fprintf(fp, " font \"%s\"", p_label->font);
		fprintf(fp, " %s", (p_label->layer==0) ? "back" : "front");
		if(p_label->noenhanced)
			fprintf(fp, " noenhanced");
		save_textcolor(fp, &(p_label->textcolor));
		if((p_label->lp_properties.flags & LP_SHOW_POINTS) == 0)
			fprintf(fp, " nopoint");
		else {
			fprintf(fp, " point");
			save_linetype(fp, &(p_label->lp_properties), true);
		}
		save_position(fp, &p_label->offset, 3, true);
#ifdef EAM_BOXED_TEXT
		if(p_label->boxed)
			fprintf(fp, " boxed ");
#endif
		fputc('\n', fp);
	}
	fputs("unset arrow\n", fp);
	for(p_arrow = first_arrow; p_arrow != NULL;
	    p_arrow = p_arrow->next) {
		fprintf(fp, "set arrow %d from ", p_arrow->tag);
		save_position(fp, &p_arrow->start, 3, false);
		if(p_arrow->type == arrow_end_absolute) {
			fputs(" to ", fp);
			save_position(fp, &p_arrow->end, 3, false);
		}
		else if(p_arrow->type == arrow_end_absolute) {
			fputs(" rto ", fp);
			save_position(fp, &p_arrow->end, 3, false);
		}
		else { /* type arrow_end_oriented */
			GpPosition * e = &p_arrow->end;
			fputs(" length ", fp);
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, " angle %g", p_arrow->angle);
		}
		fprintf(fp, " %s %s %s",
		    arrow_head_names[p_arrow->arrow_properties.head],
		    (p_arrow->arrow_properties.layer==0) ? "back" : "front",
		    (p_arrow->arrow_properties.headfill==AS_FILLED) ? "filled" :
		    (p_arrow->arrow_properties.headfill==AS_EMPTY) ? "empty" :
		    (p_arrow->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
		    "nofilled");
		save_linetype(fp, &(p_arrow->arrow_properties.lp_properties), false);
		if(p_arrow->arrow_properties.head_length > 0) {
			fprintf(fp, " size %s %.3f,%.3f,%.3f",
			    coord_msg[p_arrow->arrow_properties.head_lengthunit],
			    p_arrow->arrow_properties.head_length,
			    p_arrow->arrow_properties.head_angle,
			    p_arrow->arrow_properties.head_backangle);
		}
		fprintf(fp, "\n");
	}
#if 1 || defined(BACKWARDS_COMPATIBLE)
	fprintf(fp, "set style increment %s\n", prefer_line_styles ? "userstyles" : "default");
#endif
	fputs("unset style line\n", fp);
	for(p_line_style = first_linestyle; p_line_style != NULL;
	    p_line_style = p_line_style->next) {
		fprintf(fp, "set style line %d ", p_line_style->tag);
		save_linetype(fp, &(p_line_style->lp_properties), true);
		fprintf(fp, "\n");
	}
	/* TODO save "set linetype" as well, or instead */
	fputs("unset style arrow\n", fp);
	for(p_arrow_style = first_arrowstyle; p_arrow_style != NULL;
	    p_arrow_style = p_arrow_style->next) {
		fprintf(fp, "set style arrow %d", p_arrow_style->tag);
		fprintf(fp, " %s %s %s", arrow_head_names[p_arrow_style->arrow_properties.head],
		    (p_arrow_style->arrow_properties.layer==0) ? "back" : "front",
		    (p_arrow_style->arrow_properties.headfill==AS_FILLED) ? "filled" :
		    (p_arrow_style->arrow_properties.headfill==AS_EMPTY) ? "empty" :
		    (p_arrow_style->arrow_properties.headfill==AS_NOBORDER) ? "noborder" :
		    "nofilled");
		save_linetype(fp, &(p_arrow_style->arrow_properties.lp_properties), false);
		if(p_arrow_style->arrow_properties.head_length > 0) {
			fprintf(fp, " size %s %.3f,%.3f,%.3f",
			    coord_msg[p_arrow_style->arrow_properties.head_lengthunit],
			    p_arrow_style->arrow_properties.head_length,
			    p_arrow_style->arrow_properties.head_angle,
			    p_arrow_style->arrow_properties.head_backangle);
			if(p_arrow_style->arrow_properties.head_fixedsize) {
				fputs(" ", fp);
				fputs(" fixed", fp);
			}
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "set style histogram ");
	save_histogram_opts(fp);
#ifdef EAM_OBJECTS
	fprintf(fp, "unset object\n");
	save_object(fp, 0);
#endif
#ifdef EAM_BOXED_TEXT
	fprintf(fp, "set style textbox %s margins %4.1f, %4.1f %s\n",
	    textbox_opts.opaque ? "opaque" : "transparent",
	    textbox_opts.xmargin, textbox_opts.ymargin,
	    textbox_opts.noborder ? "noborder" : "border");
#endif
	fputs("unset logscale\n", fp);
#define SAVE_LOG(axis) if(AxA[axis].Flags & GpAxis::fLog) fprintf(fp, "set logscale %s %g\n", GetAxisName(axis), AxA[axis].base);
	SAVE_LOG(FIRST_X_AXIS);
	SAVE_LOG(FIRST_Y_AXIS);
	SAVE_LOG(SECOND_X_AXIS);
	SAVE_LOG(SECOND_Y_AXIS);
	SAVE_LOG(FIRST_Z_AXIS);
	SAVE_LOG(COLOR_AXIS);
	SAVE_LOG(POLAR_AXIS);
#undef SAVE_LOG
	SaveOffsets(fp, "set offsets");
	// FIXME 
	fprintf(fp, "set pointsize %g\nset pointintervalbox %g\nset encoding %s\n%sset polar\n%sset parametric\n",
	    PtSz, PtIntervalBox, encoding_names[encoding], (IsPolar) ? "" : "un", IsParametric ? "" : "un");
	if(numeric_locale)
		fprintf(fp, "set decimalsign locale \"%s\"\n", numeric_locale);
	if(decimalsign)
		fprintf(fp, "set decimalsign '%s'\n", decimalsign);
	if(!numeric_locale && !decimalsign)
		fprintf(fp, "unset decimalsign\n");
	fputs("set view ", fp);
	if(splot_map)
		fprintf(fp, "map scale %g", mapview_scale);
	else {
		fprintf(fp, "%g, %g, %g, %g", surface_rot_x, surface_rot_z, surface_scale, surface_zscale);
	}
	if(AspectRatio3D)
		fprintf(fp, "\nset view  %s", AspectRatio3D == 2 ? "equal xy" : AspectRatio3D == 3 ? "equal xyz" : "");
	fprintf(fp, "\nset samples %d, %d\nset isosamples %d, %d\n%sset surface %s",
	    Samples1, Samples2, iso_samples_1, iso_samples_2,
	    (draw_surface) ? "" : "un", (implicit_surface) ? "" : "explicit");
	fprintf(fp, "\n%sset contour", (draw_contour) ? "" : "un");
	switch(draw_contour) {
		case CONTOUR_NONE: fputc('\n', fp); break;
		case CONTOUR_BASE: fputs(" base\n", fp); break;
		case CONTOUR_SRF: fputs(" surface\n", fp); break;
		case CONTOUR_BOTH: fputs(" both\n", fp); break;
	}
	// Contour label options
	fprintf(fp, "set cntrlabel %s format '%s' font '%s' start %d interval %d\n",
	    clabel_onecolor ? "onecolor" : "", contour_format, NZOR(P_ClabelFont, ""), clabel_start, clabel_interval);
	fputs("set mapping ", fp);
	switch(mapping3d) {
		case MAP3D_SPHERICAL:
		    fputs("spherical\n", fp);
		    break;
		case MAP3D_CYLINDRICAL:
		    fputs("cylindrical\n", fp);
		    break;
		case MAP3D_CARTESIAN:
		default:
		    fputs("cartesian\n", fp);
		    break;
	}
	if(GpDf.missing_val)
		fprintf(fp, "set datafile missing '%s'\n", GpDf.missing_val);
	if(GpDf.df_separators)
		fprintf(fp, "set datafile separator \"%s\"\n", GpDf.df_separators);
	else
		fprintf(fp, "set datafile separator whitespace\n");
	if(strcmp(GpDf.df_commentschars, DEFAULT_COMMENTS_CHARS))
		fprintf(fp, "set datafile commentschars '%s'\n", GpDf.df_commentschars);
	if(GpDf.df_fortran_constants)
		fprintf(fp, "set datafile fortran\n");
	if(GpDf.df_nofpe_trap)
		fprintf(fp, "set datafile nofpe_trap\n");
	save_hidden3doptions(fp);
	fprintf(fp, "set cntrparam order %d\n", contour_order);
	fputs("set cntrparam ", fp);
	switch(contour_kind) {
		case CONTOUR_KIND_LINEAR:
		    fputs("linear\n", fp);
		    break;
		case CONTOUR_KIND_CUBIC_SPL:
		    fputs("cubicspline\n", fp);
		    break;
		case CONTOUR_KIND_BSPLINE:
		    fputs("bspline\n", fp);
		    break;
	}
	fputs("set cntrparam levels ", fp);
	switch(contour_levels_kind) {
		case LEVELS_AUTO:
		    fprintf(fp, "auto %d\n", contour_levels);
		    break;
		case LEVELS_INCREMENTAL:
		    fprintf(fp, "incremental %g,%g,%g\n",
		    contour_levels_list[0], contour_levels_list[1],
		    contour_levels_list[0] + contour_levels_list[1] * contour_levels);
		    break;
		case LEVELS_DISCRETE:
	    {
		    fprintf(fp, "discrete %g", contour_levels_list[0]);
		    for(int i = 1; i < contour_levels; i++)
			    fprintf(fp, ",%g ", contour_levels_list[i]);
		    fputc('\n', fp);
	    }
	}
	fprintf(fp, "set cntrparam points %d\nset size ratio %g %g,%g\nset origin %g,%g\n",
	    contour_pts, AspectRatio, XSz, YSz, XOffs, YOffs);
	fprintf(fp, "set style data ");
	save_data_func_style(fp, "data", DataStyle);
	fprintf(fp, "set style function ");
	save_data_func_style(fp, "function", FuncStyle);

	SaveZeroAxis(fp, FIRST_X_AXIS);
	SaveZeroAxis(fp, FIRST_Y_AXIS);
	SaveZeroAxis(fp, FIRST_Z_AXIS);
	SaveZeroAxis(fp, SECOND_X_AXIS);
	SaveZeroAxis(fp, SECOND_Y_AXIS);
	if(xyplane.IsAbsolute)
		fprintf(fp, "set xyplane at %g\n", xyplane.Z);
	else
		fprintf(fp, "set xyplane relative %g\n", xyplane.Z);
	{
		fprintf(fp, "set tics scale ");
		for(int i = 0; i < MAX_TICLEVEL; i++)
			fprintf(fp, " %g%c", TicScale[i], i<MAX_TICLEVEL-1 ? ',' : '\n');
	}
#define SAVE_MINI(axis)							\
	switch(AxA[axis].minitics & TICS_MASK) {			    \
		case 0: fprintf(fp, "set nom%stics\n", GetAxisName(axis)); break; \
		case MINI_AUTO: fprintf(fp, "set m%stics\n", GetAxisName(axis)); break; \
		case MINI_DEFAULT: fprintf(fp, "set m%stics default\n", GetAxisName(axis)); break; \
		case MINI_USER: fprintf(fp, "set m%stics %f\n", GetAxisName(axis), AxA[axis].mtic_freq); break; \
	}

	SAVE_MINI(FIRST_X_AXIS);
	SAVE_MINI(FIRST_Y_AXIS);
	SAVE_MINI(FIRST_Z_AXIS); /* HBB 20000506: noticed mztics were not saved! */
	SAVE_MINI(SECOND_X_AXIS);
	SAVE_MINI(SECOND_Y_AXIS);
	SAVE_MINI(COLOR_AXIS);
	SAVE_MINI(POLAR_AXIS);
#undef SAVE_MINI

	SaveTics(fp, FIRST_X_AXIS);
	SaveTics(fp, FIRST_Y_AXIS);
	SaveTics(fp, FIRST_Z_AXIS);
	SaveTics(fp, SECOND_X_AXIS);
	SaveTics(fp, SECOND_Y_AXIS);
	SaveTics(fp, COLOR_AXIS);
	SaveTics(fp, POLAR_AXIS);
	for(axis = 0; axis < (int)NumParallelAxes; axis++)
		SavePTics(fp, P_ParallelAxis[axis]);
#define SAVE_AXISLABEL_OR_TITLE(name, suffix, lab)			   \
	{								     \
		fprintf(fp, "set %s%s \"%s\" ", name, suffix, lab.text ? conv_text(lab.text) : ""); \
		fprintf(fp, "\nset %s%s ", name, suffix);			 \
		save_position(fp, &(lab.offset), 3, true);				 \
		fprintf(fp, " font \"%s\"", lab.font ? conv_text(lab.font) : ""); \
		save_textcolor(fp, &(lab.textcolor));				 \
		if(lab.tag == ROTATE_IN_3D_LABEL_TAG)				\
			fprintf(fp, " rotate parallel");			     \
		if(lab.rotate)							\
			fprintf(fp, " rotate by %d", lab.rotate);		     \
		else								 \
			fprintf(fp, " norotate");				     \
		fprintf(fp, "%s\n", (lab.noenhanced) ? " noenhanced" : "");	 \
	}
	SAVE_AXISLABEL_OR_TITLE("", "title", title);
	/* FIXME */
	fprintf(fp, "set timestamp %s \n", timelabel_bottom ? "bottom" : "top");
	SAVE_AXISLABEL_OR_TITLE("", "timestamp", timelabel);

	save_prange(fp, &AxA[POLAR_AXIS]);
	save_prange(fp, &AxA[T_AXIS]);
	save_prange(fp, &AxA[U_AXIS]);
	save_prange(fp, &AxA[V_AXIS]);

#define SAVE_AXISLABEL(axis) SAVE_AXISLABEL_OR_TITLE(GetAxisName(axis), "label", AxA[axis].label)

	SAVE_AXISLABEL(FIRST_X_AXIS);
	SAVE_AXISLABEL(SECOND_X_AXIS);
	save_prange(fp, &AxA[FIRST_X_AXIS]);
	save_prange(fp, &AxA[SECOND_X_AXIS]);

	SAVE_AXISLABEL(FIRST_Y_AXIS);
	SAVE_AXISLABEL(SECOND_Y_AXIS);
	save_prange(fp, &AxA[FIRST_Y_AXIS]);
	save_prange(fp, &AxA[SECOND_Y_AXIS]);

	SAVE_AXISLABEL(FIRST_Z_AXIS);
	save_prange(fp, &AxA[FIRST_Z_AXIS]);

	SAVE_AXISLABEL(COLOR_AXIS);
	save_prange(fp, &AxA[COLOR_AXIS]);
	for(axis = 0; axis < (int)NumParallelAxes; axis++)
		save_prange(fp, &P_ParallelAxis[axis]);
	// These will only print something if the axis is, in fact, linked
	save_link(fp, &AxA[SECOND_X_AXIS]);
	save_link(fp, &AxA[SECOND_Y_AXIS]);
	for(axis = 0; axis<NUMBER_OF_MAIN_VISIBLE_AXES; axis++)
		save_nonlinear(fp, &AxA[axis]);
#undef SAVE_AXISLABEL
#undef SAVE_AXISLABEL_OR_TITLE
	save_jitter(fp);
	fprintf(fp, "set zero %g\n", Zero);
	fprintf(fp, "set LMrg %s %g\n", LMrg.scalex == screen ? "at screen" : "", LMrg.x);
	fprintf(fp, "set BMrg %s %g\n", BMrg.scalex == screen ? "at screen" : "", BMrg.x);
	fprintf(fp, "set RMrg %s %g\n", RMrg.scalex == screen ? "at screen" : "", RMrg.x);
	fprintf(fp, "set TMrg %s %g\n", TMrg.scalex == screen ? "at screen" : "", TMrg.x);
	fprintf(fp, "set locale \"%s\"\n", get_time_locale());
	/* pm3d options */
	fputs("set pm3d ", fp);
	fputs((PM3D_IMPLICIT == Pm3D.implicit ? "implicit" : "explicit"), fp);
	fprintf(fp, " at %s\n", Pm3D.where);
	fputs("set pm3d ", fp);
	switch(Pm3D.direction) {
		case PM3D_SCANS_AUTOMATIC: fputs("scansautomatic\n", fp); break;
		case PM3D_SCANS_FORWARD: fputs("scansforward\n", fp); break;
		case PM3D_SCANS_BACKWARD: fputs("scansbackward\n", fp); break;
		case PM3D_DEPTH: fputs("depthorder\n", fp); break;
	}
	fprintf(fp, "set pm3d interpolate %d,%d", Pm3D.interp_i, Pm3D.interp_j);
	fputs(" flush ", fp);
	switch(Pm3D.flush) {
		case PM3D_FLUSH_CENTER: fputs("center", fp); break;
		case PM3D_FLUSH_BEGIN: fputs("begin", fp); break;
		case PM3D_FLUSH_END: fputs("end", fp); break;
	}
	fputs((Pm3D.ftriangles ? " " : " no"), fp);
	fputs("ftriangles", fp);
	if(Pm3D.border.l_type == LT_NODRAW) {
		fprintf(fp, " noborder");
	}
	else {
		fprintf(fp, " border");
		save_linetype(fp, &Pm3D.border, false);
	}
	fputs(" corners2color ", fp);
	switch(Pm3D.which_corner_color) {
		case PM3D_WHICHCORNER_MEAN:    fputs("mean", fp); break;
		case PM3D_WHICHCORNER_GEOMEAN: fputs("geomean", fp); break;
		case PM3D_WHICHCORNER_HARMEAN: fputs("harmean", fp); break;
		case PM3D_WHICHCORNER_MEDIAN:  fputs("median", fp); break;
		case PM3D_WHICHCORNER_MIN:     fputs("min", fp); break;
		case PM3D_WHICHCORNER_MAX:     fputs("max", fp); break;
		case PM3D_WHICHCORNER_RMS:     fputs("rms", fp); break;
		default: // PM3D_WHICHCORNER_C1 ... _C4 
		    fprintf(fp, "c%i", Pm3D.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
	}
	fputs("\n", fp);
	if(Pm3DShade.strength <= 0)
		fputs("set pm3d nolighting\n", fp);
	else
		fprintf(fp, "set pm3d lighting primary %g specular %g\n", Pm3DShade.strength, Pm3DShade.spec);
	/*
	 *  Save palette information
	 */
	fprintf(fp, "set palette %s %s maxcolors %d ",
	    SmPalette.positive==SMPAL_POSITIVE ? "positive" : "negative",
	    SmPalette.ps_allcF ? "ps_allcF" : "nops_allcF",
	    SmPalette.use_maxcolors);
	fprintf(fp, "gamma %g ", SmPalette.gamma);
	if(SmPalette.colorMode == SMPAL_COLOR_MODE_GRAY) {
		fputs("gray\n", fp);
	}
	else {
		fputs("color model ", fp);
		switch(SmPalette.cmodel) {
			case C_MODEL_RGB: fputs("RGB ", fp); break;
			case C_MODEL_HSV: fputs("HSV ", fp); break;
			case C_MODEL_CMY: fputs("CMY ", fp); break;
			case C_MODEL_YIQ: fputs("YIQ ", fp); break;
			case C_MODEL_XYZ: fputs("XYZ ", fp); break;
			default:
			    fprintf(stderr, "%s:%d ooops: Unknown color model '%c'.\n", __FILE__, __LINE__, (char)(SmPalette.cmodel) );
		}
		fputs("\nset palette ", fp);
		switch(SmPalette.colorMode) {
			case SMPAL_COLOR_MODE_RGB:
			    fprintf(fp, "rgbformulae %d, %d, %d\n", SmPalette.formulaR,
			    SmPalette.formulaG, SmPalette.formulaB);
			    break;
			case SMPAL_COLOR_MODE_GRADIENT: {
			    fprintf(fp, "defined (");
			    for(int i = 0; i < SmPalette.gradient_num; i++) {
				    fprintf(fp, " %.4g %.4g %.4g %.4g", SmPalette.gradient[i].pos,
					    SmPalette.gradient[i].col.r, SmPalette.gradient[i].col.g, SmPalette.gradient[i].col.b);
				    if(i < SmPalette.gradient_num-1) {
					    fputs(",", fp);
					    if(i==2 || i%4==2) 
							fputs("\\\n    ", fp);
				    }
			    }
			    fputs(" )\n", fp);
			    break;
		    }
			case SMPAL_COLOR_MODE_FUNCTIONS:
			    fprintf(fp, "functions %s, %s, %s\n", SmPalette.Afunc.definition,
			    SmPalette.Bfunc.definition, SmPalette.Cfunc.definition);
			    break;
			case SMPAL_COLOR_MODE_CUBEHELIX:
			    fprintf(fp, "cubehelix start %.2g cycles %.2g saturation %.2g\n",
			    SmPalette.cubehelix_start, SmPalette.cubehelix_cycles,
			    SmPalette.cubehelix_saturation);
			    break;
			default:
			    fprintf(stderr, "%s:%d ooops: Unknown color mode '%c'.\n",
			    __FILE__, __LINE__, (char)(SmPalette.colorMode) );
		}
	}

	/*
	 *  Save colorbox info
	 */
	if(ColorBox.where != SMCOLOR_BOX_NO)
		fprintf(fp, "set colorbox %s\n", ColorBox.where==SMCOLOR_BOX_DEFAULT ? "default" : "user");
	fprintf(fp, "set colorbox %sal origin ", ColorBox.rotation ==  'v' ? "vertic" : "horizont");
	save_position(fp, &ColorBox.origin, 2, false);
	fputs(" size ", fp);
	save_position(fp, &ColorBox.size, 2, false);
	fprintf(fp, " %s ", ColorBox.layer ==  LAYER_FRONT ? "front" : "back");
	if(ColorBox.border == 0) fputs("noborder", fp);
	else if(ColorBox.border_lt_tag < 0) fputs("bdefault", fp);
	else fprintf(fp, "border %d", ColorBox.border_lt_tag);
	if(ColorBox.where == SMCOLOR_BOX_NO) fputs("\nunset colorbox\n", fp);
	else fputs("\n", fp);

	fprintf(fp, "set style boxplot %s %s %5.2f %soutliers pt %d separation %g labels %s %ssorted\n",
	    boxplot_opts.plotstyle == FINANCEBARS ? "financebars" : "candles",
	    boxplot_opts.limit_type == 1 ? "fraction" : "range",
	    boxplot_opts.limit_value,
	    boxplot_opts.outliers ? "" : "no",
	    boxplot_opts.pointtype+1,
	    boxplot_opts.separation,
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X)  ? "x"  :
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2) ? "x2" :
	    (boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_AUTO) ? "auto" : "off",
	    boxplot_opts.sort_factors ? "" : "un");

	fputs("set loadpath ", fp);
	{
		char * s;
		while((s = save_loadpath()) != NULL)
			fprintf(fp, "\"%s\" ", s);
		fputc('\n', fp);
	}

	fputs("set fontpath ", fp);
	{
		char * s;
		while((s = save_fontpath()) != NULL)
			fprintf(fp, "\"%s\" ", s);
		fputc('\n', fp);
	}

	if(PS_psdir)
		fprintf(fp, "set psdir \"%s\"\n", PS_psdir);
	else
		fprintf(fp, "set psdir\n");

	fprintf(fp, "set fit");
	if(GpF.fit_suppress_log)
		fprintf(fp, " nologfile");
	else if(GpF.fitlogfile)
		fprintf(fp, " logfile \'%s\'", GpF.fitlogfile);
	switch(GpF.fit_verbosity) {
		case QUIET: fprintf(fp, " quiet"); break;
		case RESULTS: fprintf(fp, " results"); break;
		case BRIEF: fprintf(fp, " brief"); break;
		case VERBOSE: fprintf(fp, " verbose"); break;
	}
	fprintf(fp, " %serrorvariables", GpF.fit_errorvariables ? "" : "no");
	fprintf(fp, " %scovariancevariables", GpF.fit_covarvariables ? "" : "no");
	fprintf(fp, " %serrorscaling", GpF.fit_errorscaling ? "" : "no");
	fprintf(fp, " %sprescale", GpF.fit_prescale ? "" : "no");
	{
		int i;
		UdvtEntry * v = GpGg.Ev.GetUdvByName(GpF.FITLIMIT);
		double d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
		if((d > 0.) && (d < 1.))
			fprintf(fp, " limit %g", d);
		if(GpF.epsilon_abs > 0.)
			fprintf(fp, " limit_abs %g", GpF.epsilon_abs);
		v = GpGg.Ev.GetUdvByName(GpF.FITMAXITER);
		i = (v && (v->udv_value.type != NOTDEFINED)) ? real_int(&(v->udv_value)) : -1;
		if(i > 0)
			fprintf(fp, " maxiter %i", i);
		v = GpGg.Ev.GetUdvByName(GpF.FITSTARTLAMBDA);
		d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
		if(d > 0.)
			fprintf(fp, " start_lambda %g", d);
		v = GpGg.Ev.GetUdvByName(GpF.FITLAMBDAFACTOR);
		d = (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : -1.0;
		if(d > 0.)
			fprintf(fp, " lambda_factor %g", d);
	}
	if(GpF.fit_script)
		fprintf(fp, " script \'%s\'", GpF.fit_script);
	if(GpF.fit_wrap != 0)
		fprintf(fp, " wrap %i", GpF.fit_wrap);
	else
		fprintf(fp, " nowrap");
	fprintf(fp, " v%i", GpF.fit_v4compatible ? 4 : 5);
	fputc('\n', fp);
}

//static void save_ptics(FILE * fp, GpAxis * pAx)
void GpGadgets::SavePTics(FILE * fp, GpAxis & rAx)
{
	if((rAx.ticmode & TICS_MASK) == NO_TICS) {
		fprintf(fp, "unset %stics\n", GetAxisName(rAx.Index));
	}
	else {
		fprintf(fp, "set %stics %s %s scale %g,%g %smirror %s ",
			GetAxisName(rAx.Index), ((rAx.ticmode & TICS_MASK) == TICS_ON_AXIS) ? "axis" : "border",
			(rAx.Flags & GpAxis::fTicIn) ? "in" : "out", rAx.ticscale, rAx.miniticscale, (rAx.ticmode & TICS_MIRROR) ? "" : "no",
			rAx.tic_rotate ? "rotate" : "norotate");
		if(rAx.tic_rotate)
			fprintf(fp, "by %d ", rAx.tic_rotate);
		save_position(fp, &rAx.ticdef.offset, 3, true);
		if(rAx.Flags & GpAxis::fManualJustify)
			save_justification(rAx.label.pos, fp);
		else
			fputs(" autojustify", fp);
		fprintf(fp, "\nset %stics ", GetAxisName(rAx.Index));
		fprintf(fp, (rAx.ticdef.rangelimited) ? " rangelimit " : " norangelimit ");
		if(rAx.ticdef.logscaling)
			fputs("logscale ", fp);
		switch(rAx.ticdef.type) {
			case TIC_COMPUTED: {
				fputs("autofreq ", fp);
				break;
			}
			case TIC_MONTH: {
				fprintf(fp, "\nset %smtics", GetAxisName(rAx.Index));
				break;
			}
			case TIC_DAY: {
				fprintf(fp, "\nset %sdtics", GetAxisName(rAx.Index));
				break;
			}
			case TIC_SERIES:
				if(rAx.ticdef.def.series.start != -GPVL) {
					save_num_or_time_input(fp, (double)rAx.ticdef.def.series.start, &rAx);
					putc(',', fp);
				}
				fprintf(fp, "%g", rAx.ticdef.def.series.incr);
				if(rAx.ticdef.def.series.end != GPVL) {
					putc(',', fp);
					save_num_or_time_input(fp, (double)rAx.ticdef.def.series.end, &rAx);
				}
				break;
			case TIC_USER:
				break;
		}
		if(rAx.ticdef.font && *rAx.ticdef.font)
			fprintf(fp, " font \"%s\"", rAx.ticdef.font);
		if(rAx.ticdef.enhanced == false)
			fprintf(fp, " noenhanced");
		if(rAx.ticdef.textcolor.type != TC_DEFAULT)
			save_textcolor(fp, &rAx.ticdef.textcolor);
		putc('\n', fp);
		if(rAx.ticdef.def.user) {
			ticmark * t;
			fprintf(fp, "set %stics %s ", GetAxisName(rAx.Index), (rAx.ticdef.type == TIC_USER) ? "" : "add");
			fputs(" (", fp);
			for(t = rAx.ticdef.def.user; t != NULL; t = t->next) {
				if(t->level >= 0) { // Don't save ticlabels read from data file
					if(t->label)
						fprintf(fp, "\"%s\" ", conv_text(t->label));
					save_num_or_time_input(fp, (double)t->position, &rAx);
					if(t->level)
						fprintf(fp, " %d", t->level);
					if(t->next) {
						fputs(", ", fp);
					}
				}
			}
			fputs(")\n", fp);
		}
	}
}

//static void save_tics(FILE * fp, AXIS_INDEX axis)
void GpGadgets::SaveTics(FILE * fp, AXIS_INDEX axIdx)
{
	SavePTics(fp, AxA[axIdx]);
}

void save_num_or_time_input(FILE * fp, double x, GpAxis * this_axis)
{
	if(this_axis->datatype == DT_TIMEDATE) {
		char s[80];
		putc('"', fp);
		gstrftime(s, 80, GpGg.P_TimeFormat, (double)(x));
		fputs(conv_text(s), fp);
		putc('"', fp);
	}
	else {
		fprintf(fp, "%#g", x);
	}
}

void save_style_parallel(FILE * fp)
{
	fprintf(fp, "set style parallel %s ", parallel_axis_style.layer == LAYER_BACK ? "back" : "front");
	save_linetype(fp, &(parallel_axis_style.lp_properties), false);
	fprintf(fp, "\n");
}

void save_position(FILE * fp, GpPosition * pos, int ndim, bool offset)
{
	assert(first_axes == 0 && second_axes == 1 && graph == 2 && screen == 3 && character == 4);
	if(offset) {
		if(pos->x == 0 && pos->y == 0 && pos->z == 0)
			return;
		fprintf(fp, " offset ");
	}
	/* Save in time coordinates if appropriate */
	if(pos->scalex == first_axes) {
		save_num_or_time_input(fp, pos->x, &GpGg[FIRST_X_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", coord_msg[pos->scalex], pos->x);
	}
	if(ndim == 1)
		return;
	else
		fprintf(fp, ", ");
	if(pos->scaley == first_axes) {
		if(pos->scaley != pos->scalex) 
			fprintf(fp, "first ");
		save_num_or_time_input(fp, pos->y, &GpGg[FIRST_Y_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", pos->scaley == pos->scalex ? "" : coord_msg[pos->scaley], pos->y);
	}
	if(ndim == 2)
		return;
	else
		fprintf(fp, ", ");
	if(pos->scalez == first_axes) {
		if(pos->scalez != pos->scaley) 
			fprintf(fp, "first ");
		save_num_or_time_input(fp, pos->z, &GpGg[FIRST_Z_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", pos->scalez == pos->scaley ? "" : coord_msg[pos->scalez], pos->z);
	}
}

void save_prange(FILE * fp, GpAxis * pAx)
{
	fprintf(fp, "set %srange [ ", GpGg.GetAxisName(pAx->Index));
	if(pAx->SetAutoScale & AUTOSCALE_MIN) {
		if(pAx->min_constraint & CONSTRAINT_LOWER) {
			save_num_or_time_input(fp, pAx->Lb.low, pAx);
			fputs(" < ", fp);
		}
		putc('*', fp);
		if(pAx->min_constraint & CONSTRAINT_UPPER) {
			fputs(" < ", fp);
			save_num_or_time_input(fp, pAx->Ub.low, pAx);
		}
	}
	else {
		save_num_or_time_input(fp, pAx->SetRange.low, pAx);
	}
	fputs(" : ", fp);
	if(pAx->SetAutoScale & AUTOSCALE_MAX) {
		if(pAx->max_constraint & CONSTRAINT_LOWER) {
			save_num_or_time_input(fp, pAx->Lb.upp, pAx);
			fputs(" < ", fp);
		}
		putc('*', fp);
		if(pAx->max_constraint & CONSTRAINT_UPPER) {
			fputs(" < ", fp);
			save_num_or_time_input(fp, pAx->Ub.upp, pAx);
		}
	}
	else {
		save_num_or_time_input(fp, pAx->SetRange.upp, pAx);
	}
	fprintf(fp, " ] %sreverse %swriteback", ((pAx->range_flags & RANGE_IS_REVERSED)) ? "" : "no", pAx->range_flags & RANGE_WRITEBACK ? "" : "no");
	if(pAx->Index >= PARALLEL_AXES) {
		fprintf(fp, "\n");
		return;
	}
	if(pAx->SetAutoScale && fp == stderr) {
		// add current (hidden) range as comments
		fputs("  # (currently [", fp);
		if(pAx->SetAutoScale & AUTOSCALE_MIN) {
			save_num_or_time_input(fp, pAx->Range.low, pAx);
		}
		putc(':', fp);
		if(pAx->SetAutoScale & AUTOSCALE_MAX) {
			save_num_or_time_input(fp, pAx->Range.upp, pAx);
		}
		fputs("] )\n", fp);
	}
	else
		putc('\n', fp);
	if(fp != stderr) {
		if(pAx->SetAutoScale & (AUTOSCALE_FIXMIN))
			fprintf(fp, "set autoscale %sfixmin\n", GpGg.GetAxisName(pAx->Index));
		if(pAx->SetAutoScale & AUTOSCALE_FIXMAX)
			fprintf(fp, "set autoscale %sfixmax\n", GpGg.GetAxisName(pAx->Index));
	}
}

void save_link(FILE * fp, GpAxis * pAx)
{
	if(pAx->P_LinkToPrmr && pAx->Index != -pAx->P_LinkToPrmr->Index) {
		fprintf(fp, "set link %s ", GpGg.GetAxisName(pAx->Index));
		if(pAx->link_udf->at)
			fprintf(fp, "via %s ", pAx->link_udf->definition);
		if(pAx->P_LinkToPrmr->link_udf->at)
			fprintf(fp, "inverse %s ", pAx->P_LinkToPrmr->link_udf->definition);
		fputs("\n", fp);
	}
}

void save_nonlinear(FILE * fp, GpAxis * pAx)
{
#ifdef NONLINEAR_AXES
	if(pAx->P_LinkToPrmr && pAx->Index == -pAx->P_LinkToPrmr->Index) {
		fprintf(fp, "set nonlinear %s ", GpGg.GetAxisName(pAx->Index));
		if(pAx->P_LinkToPrmr->link_udf->at)
			fprintf(fp, "via %s ", pAx->P_LinkToPrmr->link_udf->definition);
		if(pAx->link_udf->at)
			fprintf(fp, "inverse %s ", pAx->link_udf->definition);
		fputs("\n", fp);
	}
#endif
}

//static void save_zeroaxis(FILE * fp, AXIS_INDEX axis)
void GpGadgets::SaveZeroAxis(FILE * fp, AXIS_INDEX axis)
{
	if(AxA[axis].zeroaxis == NULL) {
		fprintf(fp, "unset %szeroaxis", GetAxisName(axis));
	}
	else {
		fprintf(fp, "set %szeroaxis", GetAxisName(axis));
		if(AxA[axis].zeroaxis != &GpAxis::DefaultAxisZeroAxis)
			save_linetype(fp, AxA[axis].zeroaxis, false);
	}
	putc('\n', fp);
}

void save_fillstyle(FILE * fp, const fill_style_type * fs)
{
	switch(fs->fillstyle) {
		case FS_SOLID:
		case FS_TRANSPARENT_SOLID:
		    fprintf(fp, " %s solid %.2f ",
		    fs->fillstyle == FS_SOLID ? "" : "transparent",
		    fs->filldensity / 100.0);
		    break;
		case FS_PATTERN:
		case FS_TRANSPARENT_PATTERN:
		    fprintf(fp, " %s pattern %d ",
		    fs->fillstyle == FS_PATTERN ? "" : "transparent",
		    fs->fillpattern);
		    break;
		case FS_DEFAULT:
		    fprintf(fp, " default\n");
		    return;
		default:
		    fprintf(fp, " empty ");
		    break;
	}
	if(fs->border_color.type == TC_LT && fs->border_color.lt == LT_NODRAW) {
		fprintf(fp, "noborder\n");
	}
	else {
		fprintf(fp, "border");
		save_pm3dcolor(fp, &fs->border_color);
		fprintf(fp, "\n");
	}
}

void save_textcolor(FILE * fp, const t_colorspec * tc)
{
	if(tc->type) {
		fprintf(fp, " textcolor");
		if(tc->type == TC_VARIABLE)
			fprintf(fp, " variable");
		else
			save_pm3dcolor(fp, tc);
	}
}

void save_pm3dcolor(FILE * fp, const t_colorspec * tc)
{
	if(tc->type) {
		switch(tc->type) {
			case TC_LT:
				if(tc->lt == LT_NODRAW)
				    fprintf(fp, " nodraw");
			    else if(tc->lt == LT_BACKGROUND)
				    fprintf(fp, " bgnd");
			    else
				    fprintf(fp, " lt %d", tc->lt+1);
			    break;
			case TC_LINESTYLE:
				fprintf(fp, " linestyle %d", tc->lt);
			    break;
			case TC_Z:
				fprintf(fp, " palette z");
			    break;
			case TC_CB:
				fprintf(fp, " palette cb %g", tc->value);
			    break;
			case TC_FRAC:
				fprintf(fp, " palette fraction %4.2f", tc->value);
			    break;
			case TC_RGB:  {
			    const char * color = reverse_table_lookup(pm3d_color_names_tbl, tc->lt);
			    if(tc->value < 0)
				    fprintf(fp, " rgb variable ");
			    else if(color)
				    fprintf(fp, " rgb \"%s\" ", color);
			    else
				    fprintf(fp, " rgb \"#%6.6x\" ", tc->lt);
			    break;
		    }
			default:      break;
		}
	}
}

void save_data_func_style(FILE * fp, const char * which, enum PLOT_STYLE style)
{
	switch(style) {
		case LINES:
		    fputs("lines\n", fp);
		    break;
		case POINTSTYLE:
		    fputs("points\n", fp);
		    break;
		case IMPULSES:
		    fputs("impulses\n", fp);
		    break;
		case LINESPOINTS:
		    fputs("linespoints\n", fp);
		    break;
		case DOTS:
		    fputs("dots\n", fp);
		    break;
		case YERRORLINES:
		    fputs("yerrorlines\n", fp);
		    break;
		case XERRORLINES:
		    fputs("xerrorlines\n", fp);
		    break;
		case XYERRORLINES:
		    fputs("xyerrorlines\n", fp);
		    break;
		case YERRORBARS:
		    fputs("yerrorbars\n", fp);
		    break;
		case XERRORBARS:
		    fputs("xerrorbars\n", fp);
		    break;
		case XYERRORBARS:
		    fputs("xyerrorbars\n", fp);
		    break;
		case BOXES:
		    fputs("boxes\n", fp);
		    break;
		case HISTOGRAMS:
		    fputs("histograms\n", fp);
		    break;
		case FILLEDCURVES:
		    fputs("filledcurves ", fp);
		    if(!strcmp(which, "data") || !strcmp(which, "Data"))
			    filledcurves_options_tofile(&GpGg.FilledcurvesOptsData, fp);
		    else
			    filledcurves_options_tofile(&GpGg.FilledcurvesOptsFunc, fp);
		    fputc('\n', fp);
		    break;
		case BOXERROR:
		    fputs("boxerrorbars\n", fp);
		    break;
		case BOXXYERROR:
		    fputs("boxxyerrorbars\n", fp);
		    break;
		case STEPS:
		    fputs("steps\n", fp);
		    break;      /* JG */
		case FSTEPS:
		    fputs("fsteps\n", fp);
		    break;      /* HOE */
		case HISTEPS:
		    fputs("histeps\n", fp);
		    break;      /* CAC */
		case VECTOR:
		    fputs("vector\n", fp);
		    break;
		case FINANCEBARS:
		    fputs("financebars\n", fp);
		    break;
		case CANDLESTICKS:
		    fputs("candlesticks\n", fp);
		    break;
		case BOXPLOT:
		    fputs("boxplot\n", fp);
		    break;
		case PM3DSURFACE:
		    fputs("pm3d\n", fp);
		    break;
		case LABELPOINTS:
		    fputs("labels\n", fp);
		    break;
		case IMAGE:
		    fputs("image\n", fp);
		    break;
		case RGBIMAGE:
		    fputs("rgbimage\n", fp);
		    break;
#ifdef EAM_OBJECTS
		case CIRCLES:
		    fputs("circles\n", fp);
		    break;
		case ELLIPSES:
		    fputs("ellipses\n", fp);
		    break;
#endif
		case SURFACEGRID:
		    fputs("surfaces\n", fp);
		    break;
		case PARALLELPLOT:
		    fputs("parallelaxes\n", fp);
		    break;
		case PLOT_STYLE_NONE:
		default:
		    fputs("---error!---\n", fp);
	}
}

void save_dashtype(FILE * fp, int d_type, const t_dashtype * dt)
{
	// this is indicated by LT_AXIS (lt 0) instead 
	if(d_type != DASHTYPE_AXIS) {
		fprintf(fp, " dashtype");
		if(d_type == DASHTYPE_CUSTOM) {
			if(dt->dstring[0] != '\0')
				fprintf(fp, " \"%s\"", dt->dstring);
			if(fp == stderr || dt->dstring[0] == '\0') {
				fputs(" (", fp);
				for(int i = 0; i < DASHPATTERN_LENGTH && dt->pattern[i] > 0; i++)
					fprintf(fp, i ? ", %.2f" : "%.2f", dt->pattern[i]);
				fputs(")", fp);
			}
		}
		else if(d_type == DASHTYPE_SOLID) {
			fprintf(fp, " solid");
		}
		else {
			fprintf(fp, " %d", d_type + 1);
		}
	}
}

void save_linetype(FILE * fp, lp_style_type * lp, bool show_point)
{
	if(lp->l_type == LT_NODRAW)
		fprintf(fp, " lt nodraw");
	else if(lp->l_type == LT_BLACK)
		fprintf(fp, " lt black");
	else if(lp->l_type == LT_BACKGROUND)
		fprintf(fp, " lt bgnd");
	else if(lp->l_type < 0)
		fprintf(fp, " lt %d", lp->l_type+1);
	else if(lp->pm3d_color.type != TC_DEFAULT) {
		fprintf(fp, " linecolor");
		if(lp->pm3d_color.type == TC_LT)
			fprintf(fp, " %d", lp->pm3d_color.lt+1);
		else if(lp->pm3d_color.type == TC_LINESTYLE && lp->l_type == LT_COLORFROMCOLUMN)
			fprintf(fp, " variable");
		else
			save_pm3dcolor(fp, &(lp->pm3d_color));
	}
	fprintf(fp, " linewidth %.3f", lp->l_width);
	save_dashtype(fp, lp->d_type, &lp->custom_dash_pattern);
	if(show_point) {
		if(lp->p_type == PT_CHARACTER)
			fprintf(fp, " pointtype \"%s\"", lp->p_char);
		else if(lp->p_type == PT_VARIABLE)
			fprintf(fp, " pointtype variable");
		else
			fprintf(fp, " pointtype %d", lp->p_type + 1);
		if(lp->p_size == PTSZ_VARIABLE)
			fprintf(fp, " pointsize variable");
		else if(lp->p_size == PTSZ_DEFAULT)
			fprintf(fp, " pointsize default");
		else
			fprintf(fp, " pointsize %.3f", lp->p_size);
		fprintf(fp, " pointinterval %d", lp->p_interval);
	}
}

//void save_offsets(FILE * fp, char * lead)
void GpGadgets::SaveOffsets(FILE * fp, char * lead)
{
	fprintf(fp, "%s %s%g, %s%g, %s%g, %s%g\n", lead,
	    loff.scalex == graph ? "graph " : "", loff.x,
	    roff.scalex == graph ? "graph " : "", roff.x,
	    toff.scaley == graph ? "graph " : "", toff.y,
	    boff.scaley == graph ? "graph " : "", boff.y);
}

//void save_bars(FILE * fp)
void GpGadgets::SaveBars(FILE * fp)
{
	if(BarSize == 0.0) {
		fprintf(fp, "unset errorbars\n");
	}
	else {
		fprintf(fp, "set errorbars %s", (BarLayer == LAYER_BACK) ? "back" : "front");
		if(BarSize > 0.0)
			fprintf(fp, " %f ", BarSize);
		else
			fprintf(fp, " fullwidth ");
		if((BarLp.flags & LP_ERRORBAR_SET) != 0)
			save_linetype(fp, &BarLp, false);
		fputs("\n", fp);
	}
}

void save_histogram_opts(FILE * fp)
{
	switch(GpGg.histogram_opts.type) {
		default:
		case HT_CLUSTERED:
		    fprintf(fp, "clustered gap %d ", GpGg.histogram_opts.gap); break;
		case HT_ERRORBARS:
		    fprintf(fp, "errorbars gap %d lw %g", GpGg.histogram_opts.gap, GpGg.histogram_opts.bar_lw); break;
		case HT_STACKED_IN_LAYERS:
		    fprintf(fp, "rowstacked "); break;
		case HT_STACKED_IN_TOWERS:
		    fprintf(fp, "columnstacked "); break;
	}
	if(fp == stderr)
		fprintf(fp, "\n\t\t");
	fprintf(fp, "title");
	save_textcolor(fp, &GpGg.histogram_opts.title.textcolor);
	if(GpGg.histogram_opts.title.font)
		fprintf(fp, " font \"%s\" ", GpGg.histogram_opts.title.font);
	save_position(fp, &GpGg.histogram_opts.title.offset, 2, true);
	fprintf(fp, "\n");
}

#ifdef EAM_OBJECTS

/* Save/show rectangle <tag> (0 means show all) */
void save_object(FILE * fp, int tag)
{
	t_object * this_object;
	t_rectangle * this_rect;
	t_circle * this_circle;
	t_ellipse * this_ellipse;
	bool showed = false;

	for(this_object = GpGg.first_object; this_object != NULL; this_object = this_object->next) {
		if((this_object->object_type == OBJ_RECTANGLE)
		    && (tag == 0 || tag == this_object->tag)) {
			this_rect = &this_object->o.rectangle;
			showed = true;
			fprintf(fp, "%sobject %2d rect ", (fp==stderr) ? "\t" : "set ", this_object->tag);

			if(this_rect->type == 1) {
				fprintf(fp, "center ");
				save_position(fp, &this_rect->center, 2, false);
				fprintf(fp, " size ");
				save_position(fp, &this_rect->extent, 2, false);
			}
			else {
				fprintf(fp, "from ");
				save_position(fp, &this_rect->bl, 2, false);
				fprintf(fp, " to ");
				save_position(fp, &this_rect->tr, 2, false);
			}
		}

		else if((this_object->object_type == OBJ_CIRCLE)
		    && (tag == 0 || tag == this_object->tag)) {
			GpPosition * e = &this_object->o.circle.extent;
			this_circle = &this_object->o.circle;
			showed = true;
			fprintf(fp, "%sobject %2d circle ", (fp==stderr) ? "\t" : "set ", this_object->tag);

			fprintf(fp, "center ");
			save_position(fp, &this_circle->center, 3, false);
			fprintf(fp, " size ");
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, " arc [%g:%g] ", this_circle->arc_begin, this_circle->arc_end);
			fprintf(fp, this_circle->wedge ? "wedge " : "nowedge");
		}

		else if((this_object->object_type == OBJ_ELLIPSE)
		    && (tag == 0 || tag == this_object->tag)) {
			GpPosition * e = &this_object->o.ellipse.extent;
			this_ellipse = &this_object->o.ellipse;
			showed = true;
			fprintf(fp, "%sobject %2d ellipse ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			fprintf(fp, "center ");
			save_position(fp, &this_ellipse->center, 3, false);
			fprintf(fp, " size ");
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, ", %s%g", e->scaley == e->scalex ? "" : coord_msg[e->scaley], e->y);
			fprintf(fp, "  angle %g", this_ellipse->orientation);
			fputs(" units ", fp);
			switch(this_ellipse->type) {
				case ELLIPSEAXES_XY:
				    fputs("xy", fp);
				    break;
				case ELLIPSEAXES_XX:
				    fputs("xx", fp);
				    break;
				case ELLIPSEAXES_YY:
				    fputs("yy", fp);
				    break;
			}
		}

		else if((this_object->object_type == OBJ_POLYGON)
		    && (tag == 0 || tag == this_object->tag)) {
			t_polygon * this_polygon = &this_object->o.polygon;
			int nv;
			showed = true;
			fprintf(fp, "%sobject %2d polygon ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			if(this_polygon->vertex) {
				fprintf(fp, "from ");
				save_position(fp, &this_polygon->vertex[0], 3, false);
			}
			for(nv = 1; nv < this_polygon->type; nv++) {
				fprintf(fp, (fp==stderr) ? "\n\t\t\t    to " : " to ");
				save_position(fp, &this_polygon->vertex[nv], 3, false);
			}
		}

		/* Properties common to all objects */
		if(tag == 0 || tag == this_object->tag) {
			fprintf(fp, "\n%sobject %2d ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			fprintf(fp, "%s ", this_object->layer > 0 ? "front" : this_object->layer < 0 ? "behind" : "back");
			if(this_object->clip == OBJ_NOCLIP)
				fputs("noclip ", fp);
			else
				fputs("clip ", fp);
			if(this_object->lp_properties.l_width)
				fprintf(fp, "lw %.1f ", this_object->lp_properties.l_width);
			if(this_object->lp_properties.d_type)
				save_dashtype(fp, this_object->lp_properties.d_type, &this_object->lp_properties.custom_dash_pattern);
			fprintf(fp, " fc ");
			if(this_object->lp_properties.l_type == LT_DEFAULT)
				fprintf(fp, "default");
			else // FIXME: Broke with removal of use_palette? 
				save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
			fprintf(fp, " fillstyle ");
			save_fillstyle(fp, &this_object->fillstyle);
		}
	}
	if(tag > 0 && !showed)
		GpGg.IntError(GpC, GpC.CToken, "object not found");
}

#endif

