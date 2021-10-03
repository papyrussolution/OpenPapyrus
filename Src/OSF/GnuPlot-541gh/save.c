// GNUPLOT - save.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop

//static void save_functions__sub(FILE *);
//static void save_variables__sub(FILE *);
static void save_mtics(FILE *, const GpAxis * pAx);
static void save_justification(int just, FILE * fp);

const char * coord_msg[] = {"first ", "second ", "graph ", "screen ", "character ", "polar "};

static void FPutsEof(FILE * fp)
{
	fputs("#    EOF\n", fp);
}

//
// functions corresponding to the arguments of the GNUPLOT `save` command
//
//void save_functions(FILE * fp)
void GnuPlot::SaveFunctions(FILE * fp)
{
	// I _love_ information written at the top and the end of a human readable ASCII file. 
	ShowVersion(fp);
	Save_Functions__Sub(fp);
	FPutsEof(fp);
}

//void save_variables(FILE * fp)
void GnuPlot::SaveVariables(FILE * fp)
{
	ShowVersion(fp);
	Save_Variables__Sub(fp);
	FPutsEof(fp);
}

//void save_set(FILE * fp)
void GnuPlot::SaveSet(FILE * fp)
{
	ShowVersion(fp);
	SaveSetAll(fp);
	FPutsEof(fp);
}

//void save_all(FILE * fp)
void GnuPlot::SaveAll(FILE * fp)
{
	ShowVersion(fp);
	SaveSetAll(fp);
	Save_Functions__Sub(fp);
	Save_Variables__Sub(fp);
	SaveColorMaps(fp);
	SavePixmaps(fp);
	if(_Df.df_filename)
		fprintf(fp, "## Last datafile plotted: \"%s\"\n", _Df.df_filename);
	fprintf(fp, "%s\n", Pgm.replot_line);
	if(_Fit.WriToFilLastFitCmd(NULL)) {
		fputs("## ", fp);
		_Fit.WriToFilLastFitCmd(fp);
		putc('\n', fp);
	}
	FPutsEof(fp);
}

//void save_datablocks(FILE * fp)
void GnuPlot::SaveDatablocks(FILE * fp)
{
	udvt_entry * udv = Ev.P_FirstUdv->next_udv;
	while(udv) {
		if(udv->udv_value.Type == DATABLOCK) {
			char ** line = udv->udv_value.v.data_array;
			fprintf(fp, "%s << EOD\n", udv->udv_name);
			while(line && *line) {
				fprintf(fp, "%s\n", *line);
				line++;
			}
			fprintf(fp, "EOD\n");
		}
		udv = udv->next_udv;
	}
}
/*
 *  auxiliary functions
 */
//static void save_functions__sub(FILE * fp)
void GnuPlot::Save_Functions__Sub(FILE * fp)
{
	udft_entry * udf = Ev.P_FirstUdf;
	while(udf) {
		if(udf->definition)
			fprintf(fp, "%s\n", udf->definition);
		udf = udf->next_udf;
	}
}

//static void save_variables__sub(FILE * fp)
void GnuPlot::Save_Variables__Sub(FILE * fp)
{
	// always skip pi 
	udvt_entry * udv = Ev.P_FirstUdv->next_udv;
	while(udv) {
		if(udv->udv_value.Type != NOTDEFINED) {
			if((udv->udv_value.Type == ARRAY) && strncmp(udv->udv_name, "ARGV", 4)) {
				if(udv->udv_value.v.value_array[0].Type != COLORMAP_ARRAY) {
					fprintf(fp, "array %s[%d] = ", udv->udv_name, (int)(udv->udv_value.v.value_array[0].v.int_val));
					SaveArrayContent(fp, udv->udv_value.v.value_array);
				}
			}
			else if(strncmp(udv->udv_name, "GPVAL_", 6) && strncmp(udv->udv_name, "GPFUN_", 6) && strncmp(udv->udv_name, "MOUSE_", 6) && 
				strncmp(udv->udv_name, "$", 1) && (strncmp(udv->udv_name, "ARG", 3) || (strlen(udv->udv_name) != 4)) && strncmp(udv->udv_name, "NaN", 4)) {
				fprintf(fp, "%s = ", udv->udv_name);
				DispValue(fp, &(udv->udv_value), TRUE);
				putc('\n', fp);
			}
		}
		udv = udv->next_udv;
	}
}

//void save_colormaps(FILE * fp)
void GnuPlot::SaveColorMaps(FILE * fp)
{
	// always skip pi 
	udvt_entry * udv = Ev.P_FirstUdv->next_udv;
	while(udv) {
		if(udv->udv_value.Type != NOTDEFINED) {
			if(udv->udv_value.Type == ARRAY && udv->udv_value.v.value_array[0].Type == COLORMAP_ARRAY) {
				double cm_min, cm_max;
				fprintf(fp, "array %s[%d] colormap = ", udv->udv_name, (int)(udv->udv_value.v.value_array[0].v.int_val));
				SaveArrayContent(fp, udv->udv_value.v.value_array);
				get_colormap_range(udv, &cm_min, &cm_max);
				if(cm_min != cm_max)
					fprintf(fp, "set colormap %s range [%g:%g]\n", udv->udv_name, cm_min, cm_max);
			}
		}
		udv = udv->next_udv;
	}
}

//void save_array_content(FILE * fp, GpValue * array)
void GnuPlot::SaveArrayContent(FILE * fp, GpValue * pArray)
{
	int size = pArray[0].v.int_val;
	fprintf(fp, "[");
	for(int i = 1; i <= size; i++) {
		if(pArray[0].Type == COLORMAP_ARRAY)
			fprintf(fp, "0x%08x", (uint)(pArray[i].v.int_val));
		else if(pArray[i].Type != NOTDEFINED)
			DispValue(fp, &(pArray[i]), TRUE);
		if(i < size)
			fprintf(fp, ",");
	}
	fprintf(fp, "]\n");
}
//
// HBB 19990823: new function 'save term'. This will be mainly useful
// for the typical 'set term post ... plot ... set term <normal term>
// sequence. It's the only 'save' function that will write the
// current term setting to a file uncommentedly. 
//
//void save_term(FILE * fp)
void GnuPlot::SaveTerm(GpTermEntry * pTerm, FILE * fp)
{
	ShowVersion(fp);
	// A possible gotcha: the default initialization often doesn't set
	// term_options, but a 'set term <type>' without options doesn't
	// reset the options to startup defaults. This may have to be
	// changed on a per-terminal driver basis... 
	if(pTerm)
		fprintf(fp, "set terminal %s %s\n", pTerm->name, GPT.TermOptions);
	else
		fputs("set terminal unknown\n", fp);
	// output will still be written in commented form.  Otherwise, the risk of overwriting files is just too high */
	if(GPT.P_OutStr)
		fprintf(fp, "# set output '%s'\n", GPT.P_OutStr);
	else
		fputs("# set output\n", fp);
	FPutsEof(fp);
}
//
// helper function 
//
//void save_axis_label_or_title(FILE * fp, char * name, char * suffix, text_label * label, bool savejust)
void GnuPlot::SaveAxisLabelOrTitle(FILE * fp, char * pName, char * pSuffix, text_label * pLabel, bool savejust)
{
	fprintf(fp, "set %s%s \"%s\" ", pName, pSuffix, pLabel->text ? conv_text(pLabel->text) : "");
	fprintf(fp, "\nset %s%s ", pName, pSuffix);
	SavePosition(fp, &pLabel->offset, 3, TRUE);
	fprintf(fp, " font \"%s\"", pLabel->font ? conv_text(pLabel->font) : "");
	save_textcolor(fp, &pLabel->textcolor);
	if(savejust && pLabel->pos != CENTRE) 
		save_justification(pLabel->pos, fp);
	if(pLabel->tag == ROTATE_IN_3D_LABEL_TAG)
		fprintf(fp, " rotate parallel");
	else if(pLabel->rotate == TEXT_VERTICAL)
		fprintf(fp, " rotate");
	else if(pLabel->rotate)
		fprintf(fp, " rotate by %d", pLabel->rotate);
	else
		fprintf(fp, " norotate");
	if(pLabel == &Gg.LblTitle && pLabel->boxed) {
		fprintf(fp, " boxed ");
		if(pLabel->boxed > 0)
			fprintf(fp, "bs %d ", pLabel->boxed);
	}
	fprintf(fp, "%s\n", (pLabel->noenhanced) ? " noenhanced" : "");
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
void GnuPlot::SaveSetAll(FILE * fp)
{
	text_label * this_label;
	arrow_def * this_arrow;
	linestyle_def * this_linestyle;
	arrowstyle_def * this_arrowstyle;
	legend_key * key = &Gg.KeyT;
	int axis;
	// opinions are split as to whether we save term and outfile
	// as a compromise, we output them as comments !
	if(term)
		fprintf(fp, "# set terminal %s %s\n", term->name, GPT.TermOptions);
	else
		fputs("# set terminal unknown\n", fp);
	if(GPT.P_OutStr)
		fprintf(fp, "# set output '%s'\n", GPT.P_OutStr);
	else
		fputs("# set output\n", fp);
	fprintf(fp, "%sset clip points\n%sset clip one\n%sset clip two\n%sset clip radial\n", (Gg.ClipPoints ? "" : "un"), (Gg.ClipLines1 ? "" : "un"),
	    (Gg.ClipLines2 ? "" : "un"), (Gg.ClipRadial ? "" : "un"));
	SaveBars(fp);
	if(Gg.draw_border) {
		fprintf(fp, "set border %d %s", Gg.draw_border, Gg.border_layer == LAYER_BEHIND ? "behind" : Gg.border_layer == LAYER_BACK ? "back" : "front");
		save_linetype(fp, &Gg.border_lp, FALSE);
		fprintf(fp, "\n");
	}
	else
		fputs("unset border\n", fp);
	fprintf(fp, "%s cornerpoles\n", Gg.CornerPoles ? "set" : "unset");
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(axis == SAMPLE_AXIS) continue;
		if(axis == COLOR_AXIS) continue;
		if(axis == POLAR_AXIS) continue;
		fprintf(fp, "set %sdata %s\n", axis_name((AXIS_INDEX)axis), AxS[axis].datatype == DT_TIMEDATE ? "time" : AxS[axis].datatype == DT_DMS ? "geographic" : "");
	}
	if(V.BoxWidth < 0.0)
		fputs("set boxwidth\n", fp);
	else
		fprintf(fp, "set boxwidth %g %s\n", V.BoxWidth, (V.BoxWidthIsAbsolute) ? "absolute" : "relative");
	fprintf(fp, "set boxdepth %g\n", (_Plt.boxdepth > 0) ? _Plt.boxdepth : 0.0);
	fprintf(fp, "set style fill ");
	save_fillstyle(fp, &Gg.default_fillstyle);
	// Default rectangle style 
	fprintf(fp, "set style rectangle %s fc ", Gg.default_rectangle.layer > 0 ? "front" : Gg.default_rectangle.layer < 0 ? "behind" : "back");
	save_pm3dcolor(fp, &Gg.default_rectangle.lp_properties.pm3d_color);
	fprintf(fp, " fillstyle ");
	save_fillstyle(fp, &Gg.default_rectangle.fillstyle);
	// Default circle properties 
	fprintf(fp, "set style circle radius ");
	SavePosition(fp, &Gg.default_circle.o.circle.extent, 1, FALSE);
	fputs(" \n", fp);
	// Default ellipse properties 
	fprintf(fp, "set style ellipse size ");
	SavePosition(fp, &Gg.default_ellipse.o.ellipse.extent, 2, FALSE);
	fprintf(fp, " angle %g ", Gg.default_ellipse.o.ellipse.orientation);
	fputs("units ", fp);
	switch(Gg.default_ellipse.o.ellipse.type) {
		case ELLIPSEAXES_XY: fputs("xy\n", fp); break;
		case ELLIPSEAXES_XX: fputs("xx\n", fp); break;
		case ELLIPSEAXES_YY: fputs("yy\n", fp); break;
	}
	if(_Plt.dgrid3d) {
		if(_Plt.dgrid3d_mode == DGRID3D_QNORM) {
			fprintf(fp, "set dgrid3d %d,%d, %d\n", _Plt.dgrid3d_row_fineness, _Plt.dgrid3d_col_fineness, _Plt.dgrid3d_norm_value);
		}
		else if(_Plt.dgrid3d_mode == DGRID3D_SPLINES) {
			fprintf(fp, "set dgrid3d %d,%d splines\n", _Plt.dgrid3d_row_fineness, _Plt.dgrid3d_col_fineness);
		}
		else {
			fprintf(fp, "set dgrid3d %d,%d %s%s %f,%f\n", _Plt.dgrid3d_row_fineness, _Plt.dgrid3d_col_fineness,
			    reverse_table_lookup(dgrid3d_mode_tbl, _Plt.dgrid3d_mode), _Plt.dgrid3d_kdensity ? " kdensity2d" : "", _Plt.dgrid3d_x_scale, _Plt.dgrid3d_y_scale);
		}
	}
	// Dummy variable names 
	fprintf(fp, "set dummy %s", _Pb.set_dummy_var[0]);
	for(axis = 1; axis < MAX_NUM_VAR; axis++) {
		if(*_Pb.set_dummy_var[axis] == '\0')
			break;
		fprintf(fp, ", %s", _Pb.set_dummy_var[axis]);
	}
	fprintf(fp, "\n");
	AxS.SaveAxisFormat(fp, FIRST_X_AXIS);
	AxS.SaveAxisFormat(fp, FIRST_Y_AXIS);
	AxS.SaveAxisFormat(fp, SECOND_X_AXIS);
	AxS.SaveAxisFormat(fp, SECOND_Y_AXIS);
	AxS.SaveAxisFormat(fp, FIRST_Z_AXIS);
	AxS.SaveAxisFormat(fp, COLOR_AXIS);
	AxS.SaveAxisFormat(fp, POLAR_AXIS);
	fprintf(fp, "set ttics format \"%s\"\n", AxS.Theta().formatstring);
	fprintf(fp, "set timefmt \"%s\"\n", AxS.P_TimeFormat);
	fprintf(fp, "set angles %s\n", (Gg.ang2rad == 1.0) ? "radians" : "degrees");
	fprintf(fp, "set tics %s\n", AxS.grid_tics_in_front ? "front" : "back");
	if(!SomeGridSelected())
		fputs("unset grid\n", fp);
	else {
		if(AxS.polar_grid_angle) // set angle already output 
			fprintf(fp, "set grid polar %f\n", AxS.polar_grid_angle / Gg.ang2rad);
		else
			fputs("set grid nopolar\n", fp);
#define SAVE_GRID(axis) fprintf(fp, " %s%stics %sm%stics", AxS[axis].gridmajor ? "" : "no", axis_name(axis), AxS[axis].gridminor ? "" : "no", axis_name(axis));
		fputs("set grid", fp);
		SAVE_GRID(FIRST_X_AXIS);
		SAVE_GRID(FIRST_Y_AXIS);
		SAVE_GRID(FIRST_Z_AXIS);
		SAVE_GRID(POLAR_AXIS);
		fputs(" \\\n", fp);
		SAVE_GRID(SECOND_X_AXIS);
		SAVE_GRID(SECOND_Y_AXIS);
		SAVE_GRID(COLOR_AXIS);
		fputs("\n", fp);
#undef SAVE_GRID
		fprintf(fp, "set grid %s%s  ", (AxS.grid_vertical_lines) ? "vertical " : "", (AxS.grid_layer==-1) ? "layerdefault" : ((AxS.grid_layer==0) ? "back" : "front"));
		save_linetype(fp, &AxS.grid_lp, FALSE);
		fprintf(fp, ", ");
		save_linetype(fp, &AxS.mgrid_lp, FALSE);
		fputc('\n', fp);
	}
	fprintf(fp, "%sset raxis\n", AxS.raxis ? "" : "un");
	// Theta axis origin and direction 
	fprintf(fp, "set theta %s %s\n", (AxS.ThetaDirection > 0) ? "counterclockwise" : "clockwise",
	    AxS.ThetaOrigin == 180 ? "left" : AxS.ThetaOrigin ==  90 ? "top" : AxS.ThetaOrigin == -90 ? "bottom" : "right");
	// Save parallel axis state 
	SaveStyleParallel(fp);
	if(key->title.text == NULL)
		fprintf(fp, "set key notitle\n");
	else {
		fprintf(fp, "set key title \"%s\"", conv_text(key->title.text));
		if(key->title.font)
			fprintf(fp, " font \"%s\" ", key->title.font);
		save_justification(key->title.pos, fp);
		fputs("\n", fp);
	}
	fputs("set key ", fp);
	switch(key->region) {
		case GPKEY_AUTO_INTERIOR_LRTBC: fputs(key->fixed ? "fixed" : "inside", fp); break;
		case GPKEY_AUTO_EXTERIOR_LRTBC: fputs("outside", fp); break;
		case GPKEY_AUTO_EXTERIOR_MARGIN:
		    switch(key->margin) {
			    case GPKEY_TMARGIN: fputs("tmargin", fp); break;
			    case GPKEY_BMARGIN: fputs("bmargin", fp); break;
			    case GPKEY_LMARGIN: fputs("lmargin", fp); break;
			    case GPKEY_RMARGIN: fputs("rmargin", fp); break;
		    }
		    break;
		case GPKEY_USER_PLACEMENT:
		    fputs("at ", fp);
		    SavePosition(fp, &key->user_pos, 2, FALSE);
		    break;
	}
	if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && (key->margin == GPKEY_LMARGIN || key->margin == GPKEY_RMARGIN))) {
		save_justification(key->hpos, fp);
	}
	if(!(key->region == GPKEY_AUTO_EXTERIOR_MARGIN && (key->margin == GPKEY_TMARGIN || key->margin == GPKEY_BMARGIN))) {
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
	    key->auto_titles == COLUMNHEAD_KEYTITLES ? "autotitle columnhead" : key->auto_titles == FILENAME_KEYTITLES ? "autotitle" : "noautotitle");
	if(key->box.l_type > LT_NODRAW) {
		fputs("box", fp);
		save_linetype(fp, &(key->box), FALSE);
	}
	else
		fputs("nobox", fp);
	// These are for the key entries, not the key title 
	if(key->font)
		fprintf(fp, " font \"%s\"", key->font);
	if(key->textcolor.type != TC_LT || key->textcolor.lt != LT_BLACK)
		save_textcolor(fp, &key->textcolor);
	// Put less common options on separate lines 
	fprintf(fp, "\nset key %sinvert samplen %g spacing %g width %g height %g ", key->invert ? "" : "no",
	    key->swidth, key->vert_factor, key->width_fix, key->height_fix);
	fprintf(fp, "\nset key maxcolumns %d maxrows %d", key->maxcols, key->maxrows);
	fputc('\n', fp);
	if(key->front) {
		fprintf(fp, "set key opaque");
		if(key->fillcolor.lt != LT_BACKGROUND) {
			fprintf(fp, " lc ");
			save_pm3dcolor(fp, &key->fillcolor);
		}
		fprintf(fp, "\n");
	}
	else {
		fprintf(fp, "set key noopaque\n");
	}
	if(!(key->visible))
		fputs("unset key\n", fp);
	fputs("unset label\n", fp);
	for(this_label = Gg.P_FirstLabel; this_label; this_label = this_label->next) {
		fprintf(fp, "set label %d \"%s\" at ", this_label->tag, conv_text(this_label->text));
		SavePosition(fp, &this_label->place, 3, FALSE);
		if(this_label->hypertext)
			fprintf(fp, " hypertext");
		save_justification(this_label->pos, fp);
		if(this_label->rotate)
			fprintf(fp, " rotate by %d", this_label->rotate);
		else
			fprintf(fp, " norotate");
		if(this_label->font)
			fprintf(fp, " font \"%s\"", this_label->font);
		fprintf(fp, " %s", (this_label->layer==0) ? "back" : "front");
		if(this_label->noenhanced)
			fprintf(fp, " noenhanced");
		save_textcolor(fp, &(this_label->textcolor));
		if((this_label->lp_properties.flags & LP_SHOW_POINTS) == 0)
			fprintf(fp, " nopoint");
		else {
			fprintf(fp, " point");
			save_linetype(fp, &(this_label->lp_properties), TRUE);
		}
		SavePosition(fp, &this_label->offset, 3, TRUE);
		if(this_label->boxed) {
			fprintf(fp, " boxed ");
			if(this_label->boxed > 0)
				fprintf(fp, "bs %d ", this_label->boxed);
		}
		fputc('\n', fp);
	}
	fputs("unset arrow\n", fp);
	for(this_arrow = Gg.P_FirstArrow; this_arrow; this_arrow = this_arrow->next) {
		fprintf(fp, "set arrow %d from ", this_arrow->tag);
		SavePosition(fp, &this_arrow->start, 3, FALSE);
		if(this_arrow->type == arrow_end_absolute) {
			fputs(" to ", fp);
			SavePosition(fp, &this_arrow->end, 3, FALSE);
		}
		else if(this_arrow->type == arrow_end_absolute) {
			fputs(" rto ", fp);
			SavePosition(fp, &this_arrow->end, 3, FALSE);
		}
		else { /* type arrow_end_oriented */
			GpPosition * e = &this_arrow->end;
			fputs(" length ", fp);
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, " angle %g", this_arrow->angle);
		}
		fprintf(fp, " %s %s %s", arrow_head_names[this_arrow->arrow_properties.head], (this_arrow->arrow_properties.layer==0) ? "back" : "front",
		    (this_arrow->arrow_properties.headfill==AS_FILLED) ? "filled" : (this_arrow->arrow_properties.headfill==AS_EMPTY) ? "empty" :
		    (this_arrow->arrow_properties.headfill==AS_NOBORDER) ? "noborder" : "nofilled");
		save_linetype(fp, &(this_arrow->arrow_properties.lp_properties), FALSE);
		if(this_arrow->arrow_properties.head_length > 0) {
			fprintf(fp, " size %s %.3f,%.3f,%.3f", coord_msg[this_arrow->arrow_properties.head_lengthunit], this_arrow->arrow_properties.head_length,
			    this_arrow->arrow_properties.head_angle, this_arrow->arrow_properties.head_backangle);
		}
		fprintf(fp, "\n");
	}
	// Mostly for backwards compatibility 
	if(Gg.PreferLineStyles)
		fprintf(fp, "set style increment userstyles\n");
	fputs("unset style line\n", fp);
	for(this_linestyle = Gg.P_FirstLineStyle; this_linestyle; this_linestyle = this_linestyle->next) {
		fprintf(fp, "set style line %d ", this_linestyle->tag);
		save_linetype(fp, &(this_linestyle->lp_properties), TRUE);
		fprintf(fp, "\n");
	}
	fputs("unset style arrow\n", fp);
	for(this_arrowstyle = Gg.P_FirstArrowStyle; this_arrowstyle; this_arrowstyle = this_arrowstyle->next) {
		fprintf(fp, "set style arrow %d", this_arrowstyle->tag);
		fprintf(fp, " %s %s %s", arrow_head_names[this_arrowstyle->arrow_properties.head], (this_arrowstyle->arrow_properties.layer==0) ? "back" : "front",
		    (this_arrowstyle->arrow_properties.headfill==AS_FILLED) ? "filled" : (this_arrowstyle->arrow_properties.headfill==AS_EMPTY) ? "empty" :
		    (this_arrowstyle->arrow_properties.headfill==AS_NOBORDER) ? "noborder" : "nofilled");
		save_linetype(fp, &(this_arrowstyle->arrow_properties.lp_properties), FALSE);
		if(this_arrowstyle->arrow_properties.head_length > 0) {
			fprintf(fp, " size %s %.3f,%.3f,%.3f", coord_msg[this_arrowstyle->arrow_properties.head_lengthunit], this_arrowstyle->arrow_properties.head_length,
			    this_arrowstyle->arrow_properties.head_angle, this_arrowstyle->arrow_properties.head_backangle); 
			if(this_arrowstyle->arrow_properties.head_fixedsize) {
				fputs(" ", fp);
				fputs(" fixed", fp);
			}
		}
		fprintf(fp, "\n");
	}
	fprintf(fp, "set style histogram ");
	SaveHistogramOpts(fp);
	fprintf(fp, "unset object\n");
	SaveObject(fp, 0);
	fprintf(fp, "unset walls\n");
	SaveWalls(fp);
	SaveStyleTextBox(fp);
	SaveOffsets(fp, "set offsets");
	fprintf(fp, "set pointsize %g\nset pointintervalbox %g\nset encoding %s\n%sset polar\n%sset parametric\n", Gg.PointSize, Gg.PointIntervalBox,
	    encoding_names[GPT._Encoding], (Gg.Polar ? "" : "un"), (Gg.Parametric ? "" : "un"));
	if(Gg.SpiderPlot) {
		fprintf(fp, "set spiderplot\n");
		SaveStyleSpider(fp);
	}
	else
		fprintf(fp, "unset spiderplot\n");
	if(GpU.numeric_locale)
		fprintf(fp, "set decimalsign locale \"%s\"\n", GpU.numeric_locale);
	if(GpU.decimalsign)
		fprintf(fp, "set decimalsign '%s'\n", GpU.decimalsign);
	if(!GpU.numeric_locale && !GpU.decimalsign)
		fprintf(fp, "unset decimalsign\n");
	fprintf(fp, "%sset micro\n", GpU.use_micro ? "" : "un");
	fprintf(fp, "%sset minussign\n", GpU.use_minus_sign ? "" : "un");
	fputs("set view ", fp);
	if(_3DBlk.splot_map)
		fprintf(fp, "map scale %g", _3DBlk.MapviewScale);
	else if(_3DBlk.xz_projection)
		fprintf(fp, "projection xz");
	else if(_3DBlk.yz_projection)
		fprintf(fp, "projection yz");
	else {
		fprintf(fp, "%g, %g, %g, %g", _3DBlk.SurfaceRotX, _3DBlk.SurfaceRotZ, _3DBlk.SurfaceScale, _3DBlk.SurfaceZScale);
		fprintf(fp, "\nset view azimuth %g", _3DBlk.Azimuth);
	}
	if(V.AspectRatio3D)
		fprintf(fp, "\nset view  %s", (V.AspectRatio3D == 2) ? "equal xy" : ((V.AspectRatio3D == 3) ? "equal xyz" : ""));
	fprintf(fp, "\nset rgbmax %g", Gr.RgbMax);
	fprintf(fp, "\nset samples %d, %d\nset isosamples %d, %d\n%sset surface %s", Gg.Samples1, Gg.Samples2, Gg.IsoSamples1, Gg.IsoSamples2,
	    (_3DBlk.draw_surface) ? "" : "un", (_3DBlk.implicit_surface) ? "" : "explicit");
	fprintf(fp, "\n%sset contour", (_3DBlk.draw_contour) ? "" : "un");
	switch(_3DBlk.draw_contour) {
		case CONTOUR_NONE: fputc('\n', fp); break;
		case CONTOUR_BASE: fputs(" base\n", fp); break;
		case CONTOUR_SRF: fputs(" surface\n", fp); break;
		case CONTOUR_BOTH: fputs(" both\n", fp); break;
	}
	// Contour label options 
	fprintf(fp, "set cntrlabel %s format '%s' font '%s' start %d interval %d\n", _3DBlk.clabel_onecolor ? "onecolor" : "", _Cntr.ContourFormat,
	    _3DBlk.clabel_font ? _3DBlk.clabel_font : "", _3DBlk.clabel_start, _3DBlk.clabel_interval);
	fputs("set mapping ", fp);
	switch(_Plt.mapping3d) {
		case MAP3D_SPHERICAL: fputs("spherical\n", fp); break;
		case MAP3D_CYLINDRICAL: fputs("cylindrical\n", fp); break;
		case MAP3D_CARTESIAN: 
		default: fputs("cartesian\n", fp); break;
	}
	if(_Df.missing_val)
		fprintf(fp, "set datafile missing '%s'\n", _Df.missing_val);
	if(_Df.df_separators)
		fprintf(fp, "set datafile separator \"%s\"\n", _Df.df_separators);
	else
		fprintf(fp, "set datafile separator whitespace\n");
	if(strcmp(_Df.df_commentschars, DEFAULT_COMMENTS_CHARS))
		fprintf(fp, "set datafile commentschars '%s'\n", _Df.df_commentschars);
	if(_Df.df_fortran_constants)
		fprintf(fp, "set datafile fortran\n");
	if(_Df.df_nofpe_trap)
		fprintf(fp, "set datafile nofpe_trap\n");
	fprintf(fp, "set datafile %scolumnheaders\n", _Df.df_columnheaders ? "" : "no");
	SaveHidden3DOptions(fp);
	fprintf(fp, "set cntrparam order %d\n", _Cntr.ContourOrder);
	fputs("set cntrparam ", fp);
	switch(_Cntr.ContourKind) {
		case CONTOUR_KIND_LINEAR: fputs("linear\n", fp); break;
		case CONTOUR_KIND_CUBIC_SPL: fputs("cubicspline\n", fp); break;
		case CONTOUR_KIND_BSPLINE: fputs("bspline\n", fp); break;
	}
	fprintf(fp, "set cntrparam levels %d\nset cntrparam levels ", _Cntr.ContourLevels);
	switch(_Cntr.ContourLevelsKind) {
		case LEVELS_AUTO:
		    fprintf(fp, "auto");
		    break;
		case LEVELS_INCREMENTAL:
		    fprintf(fp, "incremental %g,%g",
			contour_levels_list[0], contour_levels_list[1]);
		    break;
		case LEVELS_DISCRETE:
	    {
		    fprintf(fp, "discrete %g", contour_levels_list[0]);
		    for(int i = 1; i < _Cntr.ContourLevels; i++)
			    fprintf(fp, ",%g ", contour_levels_list[i]);
	    }
	}
	fprintf(fp, "\nset cntrparam firstlinetype %d", _Cntr.ContourFirstLineType);
	fprintf(fp, " %ssorted\n", _Cntr.ContourSortLevels ? "" : "un");
	fprintf(fp, "set cntrparam points %d\nset size ratio %g %g,%g\nset origin %g,%g\n", _Cntr.ContourPts, V.AspectRatio, V.Size.x, V.Size.y, V.Offset.x, V.Offset.y);
	fprintf(fp, "set style data ");
	SaveDataFuncStyle(fp, "data", Gg.data_style);
	fprintf(fp, "set style function ");
	SaveDataFuncStyle(fp, "function", Gg.func_style);
	SaveZeroAxis(fp, FIRST_X_AXIS);
	SaveZeroAxis(fp, FIRST_Y_AXIS);
	SaveZeroAxis(fp, FIRST_Z_AXIS);
	SaveZeroAxis(fp, SECOND_X_AXIS);
	SaveZeroAxis(fp, SECOND_Y_AXIS);
	if(_3DBlk.xyplane.absolute)
		fprintf(fp, "set xyplane at %g\n", _3DBlk.xyplane.z);
	else
		fprintf(fp, "set xyplane relative %g\n", _3DBlk.xyplane.z);
	{
		fprintf(fp, "set tics scale ");
		for(int i = 0; i < MAX_TICLEVEL; i++)
			fprintf(fp, " %g%c", AxS.ticscale[i], i<MAX_TICLEVEL-1 ? ',' : '\n');
	}
	save_mtics(fp, &AxS[FIRST_X_AXIS]);
	save_mtics(fp, &AxS[FIRST_Y_AXIS]);
	save_mtics(fp, &AxS[FIRST_Z_AXIS]);
	save_mtics(fp, &AxS[SECOND_X_AXIS]);
	save_mtics(fp, &AxS[SECOND_Y_AXIS]);
	save_mtics(fp, &AxS[COLOR_AXIS]);
	save_mtics(fp, &AxS.__R());
	save_mtics(fp, &AxS.Theta());
	SaveTics(fp, &AxS[FIRST_X_AXIS]);
	SaveTics(fp, &AxS[FIRST_Y_AXIS]);
	SaveTics(fp, &AxS[FIRST_Z_AXIS]);
	SaveTics(fp, &AxS[SECOND_X_AXIS]);
	SaveTics(fp, &AxS[SECOND_Y_AXIS]);
	SaveTics(fp, &AxS[COLOR_AXIS]);
	SaveTics(fp, &AxS.__R());
	SaveTics(fp, &AxS.Theta());
	for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++)
		SaveTics(fp, &AxS.Parallel(axis));
	SaveAxisLabelOrTitle(fp, "", "title", &Gg.LblTitle, TRUE);
	fprintf(fp, "set timestamp %s \n", Gg.TimeLabelBottom ? "bottom" : "top");
	SaveAxisLabelOrTitle(fp, "", "timestamp", &Gg.LblTime, FALSE);
	SavePRange(fp, AxS[T_AXIS]);
	SavePRange(fp, AxS[U_AXIS]);
	SavePRange(fp, AxS[V_AXIS]);
#define SAVE_AXISLABEL(axis) SaveAxisLabelOrTitle(fp, axis_name(axis), "label", &AxS[axis].label, TRUE)
	SAVE_AXISLABEL(FIRST_X_AXIS);
	SAVE_AXISLABEL(SECOND_X_AXIS);
	SavePRange(fp, AxS[FIRST_X_AXIS]);
	SavePRange(fp, AxS[SECOND_X_AXIS]);
	SAVE_AXISLABEL(FIRST_Y_AXIS);
	SAVE_AXISLABEL(SECOND_Y_AXIS);
	SavePRange(fp, AxS[FIRST_Y_AXIS]);
	SavePRange(fp, AxS[SECOND_Y_AXIS]);
	SAVE_AXISLABEL(FIRST_Z_AXIS);
	SavePRange(fp, AxS[FIRST_Z_AXIS]);
	SAVE_AXISLABEL(COLOR_AXIS);
	SavePRange(fp, AxS[COLOR_AXIS]);
	SAVE_AXISLABEL(POLAR_AXIS);
	SavePRange(fp, AxS[POLAR_AXIS]);
	for(axis = 0; axis < AxS.GetParallelAxisCount(); axis++) {
		GpAxis & r_paxis = AxS.Parallel(axis);
		SavePRange(fp, r_paxis);
		if(r_paxis.label.text)
			SaveAxisLabelOrTitle(fp, axis_name((AXIS_INDEX)r_paxis.index), "label", &r_paxis.label, TRUE);
		if(r_paxis.zeroaxis) {
			fprintf(fp, "set paxis %d", axis+1);
			save_linetype(fp, r_paxis.zeroaxis, FALSE);
			fprintf(fp, "\n");
		}
	}
#undef SAVE_AXISLABEL
	fputs("unset logscale\n", fp);
	for(axis = 0; axis < NUMBER_OF_MAIN_VISIBLE_AXES; axis++) {
		if(AxS[axis].log)
			fprintf(fp, "set logscale %s %g\n", axis_name((AXIS_INDEX)axis), AxS[axis].base);
		else
			save_nonlinear(fp, &AxS[axis]);
	}
	// These will only print something if the axis is, in fact, linked 
	save_link(fp, &AxS[SECOND_X_AXIS]);
	save_link(fp, &AxS[SECOND_Y_AXIS]);
	SaveJitter(fp);
	fprintf(fp, "set zero %g\n", Gg.Zero);
	fprintf(fp, "set lmargin %s %g\n", V.MarginL.scalex == screen ? "at screen" : "", V.MarginL.x);
	fprintf(fp, "set bmargin %s %g\n", V.MarginB.scalex == screen ? "at screen" : "", V.MarginB.x);
	fprintf(fp, "set rmargin %s %g\n", V.MarginR.scalex == screen ? "at screen" : "", V.MarginR.x);
	fprintf(fp, "set tmargin %s %g\n", V.MarginT.scalex == screen ? "at screen" : "", V.MarginT.x);
	fprintf(fp, "set locale \"%s\"\n", LocaleHandler(ACTION_GET, NULL));
	// pm3d options 
	fputs("set pm3d ", fp);
	fputs((PM3D_IMPLICIT == _Pm3D.pm3d.implicit ? "implicit" : "explicit"), fp);
	fprintf(fp, " at %s\n", _Pm3D.pm3d.where);
	fputs("set pm3d ", fp);
	switch(_Pm3D.pm3d.direction) {
		case PM3D_SCANS_AUTOMATIC: fputs("scansautomatic\n", fp); break;
		case PM3D_SCANS_FORWARD: fputs("scansforward\n", fp); break;
		case PM3D_SCANS_BACKWARD: fputs("scansbackward\n", fp); break;
		case PM3D_DEPTH: fprintf(fp, "depthorder %s\n", _Pm3D.pm3d.base_sort ? "base" : ""); break;
	}
	fprintf(fp, "set pm3d interpolate %d,%d", _Pm3D.pm3d.interp_i, _Pm3D.pm3d.interp_j);
	fputs(" flush ", fp);
	switch(_Pm3D.pm3d.flush) {
		case PM3D_FLUSH_CENTER: fputs("center", fp); break;
		case PM3D_FLUSH_BEGIN: fputs("begin", fp); break;
		case PM3D_FLUSH_END: fputs("end", fp); break;
	}
	fputs((_Pm3D.pm3d.ftriangles ? " " : " no"), fp);
	fputs("ftriangles", fp);
	if(_Pm3D.pm3d.border.l_type == LT_NODRAW) {
		fprintf(fp, " noborder");
	}
	else {
		fprintf(fp, " border");
		save_linetype(fp, &_Pm3D.pm3d.border, FALSE);
	}
	fputs(" corners2color ", fp);
	switch(_Pm3D.pm3d.which_corner_color) {
		case PM3D_WHICHCORNER_MEAN:    fputs("mean", fp); break;
		case PM3D_WHICHCORNER_GEOMEAN: fputs("geomean", fp); break;
		case PM3D_WHICHCORNER_HARMEAN: fputs("harmean", fp); break;
		case PM3D_WHICHCORNER_MEDIAN:  fputs("median", fp); break;
		case PM3D_WHICHCORNER_MIN:     fputs("min", fp); break;
		case PM3D_WHICHCORNER_MAX:     fputs("max", fp); break;
		case PM3D_WHICHCORNER_RMS:     fputs("rms", fp); break;
		default: /* PM3D_WHICHCORNER_C1 ... _C4 */
		    fprintf(fp, "c%i", _Pm3D.pm3d.which_corner_color - PM3D_WHICHCORNER_C1 + 1);
	}
	fputs("\n", fp);
	fprintf(fp, "set pm3d %s %s\n", _Pm3D.pm3d.clip == PM3D_CLIP_1IN ? "clip1in" :
	    _Pm3D.pm3d.clip == PM3D_CLIP_4IN ? "clip4in" : "clip z", _Pm3D.pm3d.no_clipcb ? "noclipcb" : "");
	if(_Pm3D.pm3d_shade.strength <= 0)
		fputs("set pm3d nolighting\n", fp);
	else
		fprintf(fp, "set pm3d lighting primary %g specular %g spec2 %g\n", _Pm3D.pm3d_shade.strength, _Pm3D.pm3d_shade.spec, _Pm3D.pm3d_shade.spec2);
	//
	// Save palette information
	//
	fprintf(fp, "set palette %s %s maxcolors %d ", (SmPltt.Positive == SMPAL_POSITIVE) ? "positive" : "negative",
	    SmPltt.ps_allcF ? "ps_allcF" : "nops_allcF", SmPltt.UseMaxColors);
	fprintf(fp, "gamma %g ", SmPltt.gamma);
	if(SmPltt.colorMode == SMPAL_COLOR_MODE_GRAY) {
		fputs("gray\n", fp);
	}
	else {
		fputs("color model ", fp);
		switch(SmPltt.CModel) {
			default:
			case C_MODEL_RGB: fputs("RGB ", fp); break;
			case C_MODEL_CMY: fputs("CMY ", fp); break;
			case C_MODEL_HSV: fputs("HSV ", fp); break;
		}
		if(SmPltt.CModel == C_MODEL_HSV && SmPltt.HSV_offset != 0)
			fprintf(fp, "start %.2f ", SmPltt.HSV_offset);
		fputs("\nset palette ", fp);
		switch(SmPltt.colorMode) {
			default:
			case SMPAL_COLOR_MODE_RGB:
			    fprintf(fp, "rgbformulae %d, %d, %d\n", SmPltt.formulaR,
				SmPltt.formulaG, SmPltt.formulaB);
			    break;
			case SMPAL_COLOR_MODE_GRADIENT: {
			    int i = 0;
			    fprintf(fp, "defined (");
			    for(i = 0; i<SmPltt.GradientNum; i++) {
				    fprintf(fp, " %.4g %.4g %.4g %.4g", SmPltt.P_Gradient[i].pos,
					SmPltt.P_Gradient[i].col.r, SmPltt.P_Gradient[i].col.g, SmPltt.P_Gradient[i].col.b);
				    if(i < SmPltt.GradientNum-1) {
					    fputs(",", fp);
					    if(i==2 || i%4==2) 
							fputs("\\\n    ", fp);
				    }
			    }
			    fputs(" )\n", fp);
			    break;
		    }
			case SMPAL_COLOR_MODE_FUNCTIONS:
			    fprintf(fp, "functions %s, %s, %s\n", SmPltt.Afunc.definition, SmPltt.Bfunc.definition, SmPltt.Cfunc.definition);
			    break;
			case SMPAL_COLOR_MODE_CUBEHELIX:
			    fprintf(fp, "cubehelix start %.2g cycles %.2g saturation %.2g\n",
					SmPltt.cubehelix_start, SmPltt.cubehelix_cycles, SmPltt.cubehelix_saturation);
			    break;
		}
	}
	/*
	 *  Save colorbox info
	 */
	if(Gg.ColorBox.where != SMCOLOR_BOX_NO)
		fprintf(fp, "set colorbox %s\n", Gg.ColorBox.where==SMCOLOR_BOX_DEFAULT ? "default" : "user");
	fprintf(fp, "set colorbox %sal origin ", Gg.ColorBox.rotation ==  'v' ? "vertic" : "horizont");
	SavePosition(fp, &Gg.ColorBox.origin, 2, FALSE);
	fputs(" size ", fp);
	SavePosition(fp, &Gg.ColorBox.size, 2, FALSE);
	fprintf(fp, " %s ", Gg.ColorBox.layer ==  LAYER_FRONT ? "front" : "back");
	fprintf(fp, " %sinvert ", Gg.ColorBox.invert ? "" : "no");
	if(Gg.ColorBox.border == 0)
		fputs("noborder", fp);
	else
		fprintf(fp, "border %d", Gg.ColorBox.border_lt_tag);
	fprintf(fp, " cbtics %d", Gg.ColorBox.cbtics_lt_tag);
	if(Gg.ColorBox.where == SMCOLOR_BOX_NO) 
		fputs("\nunset colorbox\n", fp);
	else 
		fputs("\n", fp);
	fprintf(fp, "set style boxplot %s %s %5.2f %soutliers pt %d separation %g labels %s %ssorted\n",
	    Gg.boxplot_opts.plotstyle == FINANCEBARS ? "financebars" : "candles",
	    Gg.boxplot_opts.limit_type == 1 ? "fraction" : "range",
	    Gg.boxplot_opts.limit_value,
	    Gg.boxplot_opts.outliers ? "" : "no",
	    Gg.boxplot_opts.pointtype+1,
	    Gg.boxplot_opts.separation,
	    (Gg.boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X)  ? "x"  :
	    (Gg.boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_X2) ? "x2" :
	    (Gg.boxplot_opts.labels == BOXPLOT_FACTOR_LABELS_AUTO) ? "auto" : "off",
	    Gg.boxplot_opts.sort_factors ? "" : "un");
	fputs("set loadpath ", fp);
	{
		char * s;
		while((s = save_loadpath()) != NULL)
			fprintf(fp, "\"%s\" ", s);
		fputc('\n', fp);
	}
	if(PS_fontpath)
		fprintf(fp, "set fontpath \"%s\"\n", PS_fontpath);
	else
		fprintf(fp, "set fontpath\n");
	if(PS_psdir)
		fprintf(fp, "set psdir \"%s\"\n", PS_psdir);
	else
		fprintf(fp, "set psdir\n");
	fprintf(fp, "set fit");
	if(_Fit.fit_suppress_log)
		fprintf(fp, " nologfile");
	else if(_Fit.fitlogfile)
		fprintf(fp, " logfile \'%s\'", _Fit.fitlogfile);
	fprintf(fp, " %s", reverse_table_lookup(fit_verbosity_level, _Fit.fit_verbosity));
	fprintf(fp, " %serrorvariables", _Fit.fit_errorvariables ? "" : "no");
	fprintf(fp, " %scovariancevariables", _Fit.fit_covarvariables ? "" : "no");
	fprintf(fp, " %serrorscaling", _Fit.fit_errorscaling ? "" : "no");
	fprintf(fp, " %sprescale", _Fit.fit_prescale ? "" : "no");
	{
		int i;
		udvt_entry * v = Ev.GetUdvByName((char *)FITLIMIT);
		double d = (v && (v->udv_value.Type != NOTDEFINED)) ? Real(&v->udv_value) : -1.0;
		if(d > 0.0 && d < 1.0)
			fprintf(fp, " limit %g", d);
		if(_Fit.epsilon_abs > 0.)
			fprintf(fp, " limit_abs %g", _Fit.epsilon_abs);
		v = Ev.GetUdvByName((char *)FITMAXITER);
		i = (v && (v->udv_value.Type != NOTDEFINED)) ? static_cast<int>(Real(&v->udv_value)) : -1;
		if(i > 0)
			fprintf(fp, " maxiter %i", i);
		v = Ev.GetUdvByName((char *)FITSTARTLAMBDA);
		d = (v && (v->udv_value.Type != NOTDEFINED)) ? Real(&v->udv_value) : -1.0;
		if(d > 0.)
			fprintf(fp, " start_lambda %g", d);
		v = Ev.GetUdvByName((char *)FITLAMBDAFACTOR);
		d = (v && (v->udv_value.Type != NOTDEFINED)) ? Real(&v->udv_value) : -1.0;
		if(d > 0.)
			fprintf(fp, " lambda_factor %g", d);
	}
	if(_Fit.fit_script)
		fprintf(fp, " script \'%s\'", _Fit.fit_script);
	if(_Fit.fit_wrap)
		fprintf(fp, " wrap %i", _Fit.fit_wrap);
	else
		fprintf(fp, " nowrap");
	fprintf(fp, " v%i", _Fit.fit_v4compatible ? 4 : 5);
	fputc('\n', fp);
}

//static void save_tics(FILE * fp, const GpAxis * pAx)
void GnuPlot::SaveTics(FILE * fp, const GpAxis * pAx)
{
	if((pAx->ticmode & TICS_MASK) == NO_TICS) {
		fprintf(fp, "unset %stics\n", axis_name((AXIS_INDEX)pAx->index));
		return;
	}
	fprintf(fp, "set %stics %s %s scale %g,%g %smirror %s ", axis_name((AXIS_INDEX)pAx->index), 
		((pAx->ticmode & TICS_MASK) == TICS_ON_AXIS) ? "axis" : "border", (pAx->TicIn) ? "in" : "out",
	    pAx->ticscale, pAx->miniticscale, (pAx->ticmode & TICS_MIRROR) ? "" : "no", pAx->tic_rotate ? "rotate" : "norotate");
	if(pAx->tic_rotate)
		fprintf(fp, "by %d ", pAx->tic_rotate);
	SavePosition(fp, &pAx->ticdef.offset, 3, TRUE);
	if(pAx->manual_justify)
		save_justification(pAx->tic_pos, fp);
	else
		fputs(" autojustify", fp);
	fprintf(fp, "\nset %stics ", axis_name((AXIS_INDEX)pAx->index));
	fprintf(fp, (pAx->ticdef.rangelimited) ? " rangelimit " : " norangelimit ");
	if(pAx->ticdef.logscaling)
		fputs("logscale ", fp);
	switch(pAx->ticdef.type) {
		case TIC_COMPUTED: fputs("autofreq ", fp); break;
		case TIC_MONTH: fprintf(fp, "\nset %smtics", axis_name((AXIS_INDEX)pAx->index)); break;
		case TIC_DAY: fprintf(fp, "\nset %sdtics", axis_name((AXIS_INDEX)pAx->index)); break;
		case TIC_SERIES:
		    if(pAx->ticdef.def.series.start != -VERYLARGE) {
			    SaveNumOrTimeInput(fp, (double)pAx->ticdef.def.series.start, pAx);
			    putc(',', fp);
		    }
		    fprintf(fp, "%g", pAx->ticdef.def.series.incr);
		    if(pAx->ticdef.def.series.end != VERYLARGE) {
			    putc(',', fp);
			    SaveNumOrTimeInput(fp, (double)pAx->ticdef.def.series.end, pAx);
		    }
		    break;
		case TIC_USER:
		    break;
	}
	if(pAx->ticdef.font && *pAx->ticdef.font)
		fprintf(fp, " font \"%s\"", pAx->ticdef.font);
	if(pAx->ticdef.enhanced == FALSE)
		fprintf(fp, " noenhanced");
	if(pAx->ticdef.textcolor.type != TC_DEFAULT)
		save_textcolor(fp, &pAx->ticdef.textcolor);
	putc('\n', fp);
	if(pAx->ticdef.def.user) {
		ticmark * t;
		fprintf(fp, "set %stics %s ", axis_name((AXIS_INDEX)pAx->index), (pAx->ticdef.type == TIC_USER) ? "" : "add");
		fputs(" (", fp);
		for(t = pAx->ticdef.def.user; t; t = t->next) {
			if(t->level < 0) /* Don't save ticlabels read from data file */
				continue;
			if(t->label)
				fprintf(fp, "\"%s\" ", conv_text(t->label));
			SaveNumOrTimeInput(fp, (double)t->position, pAx);
			if(t->level)
				fprintf(fp, " %d", t->level);
			if(t->next) {
				fputs(", ", fp);
			}
		}
		fputs(")\n", fp);
	}
}

static void save_mtics(FILE * fp, const GpAxis * pAx)
{
	char * name = axis_name((AXIS_INDEX)pAx->index);
	switch(pAx->minitics & TICS_MASK) {
		case 0: fprintf(fp, "set nom%stics\n", name); break;
		case MINI_AUTO: fprintf(fp, "set m%stics\n", name); break;
		case MINI_DEFAULT: fprintf(fp, "set m%stics default\n", name); break;
		case MINI_USER: fprintf(fp, "set m%stics %f\n", name, pAx->mtic_freq); break;
	}
}

//void save_num_or_time_input(FILE * fp, double x, const GpAxis * pAx)
void GnuPlot::SaveNumOrTimeInput(FILE * fp, double x, const GpAxis * pAx)
{
	if(pAx->datatype == DT_TIMEDATE) {
		char s[80];
		putc('"', fp);
		GStrFTime(s, 80, AxS.P_TimeFormat, (double)(x));
		fputs(conv_text(s), fp);
		putc('"', fp);
	}
	else
		fprintf(fp, "%#g", x);
}

//void save_axis_format(FILE * fp, AXIS_INDEX axis)
void GpAxisSet::SaveAxisFormat(FILE * fp, AXIS_INDEX axis) const
{
	fprintf(fp, (fp == stderr) ? "\t  %s-axis: \"%s\"%s\n" : "set format %s \"%s\" %s\n",
	    axis_name(axis), conv_text(AxArray[axis].formatstring), AxArray[axis].tictype == DT_DMS ? "geographic" : AxArray[axis].tictype == DT_TIMEDATE ? "timedate" : "");
}

//void save_style_parallel(FILE * fp)
void GnuPlot::SaveStyleParallel(FILE * fp)
{
	if(fp == stderr)
		fputs("\t", fp);
	fprintf(fp, "set style parallel %s ", Gg.ParallelAxisStyle.layer == LAYER_BACK ? "back" : "front");
	save_linetype(fp, &(Gg.ParallelAxisStyle.lp_properties), FALSE);
	fprintf(fp, "\n");
}

//void save_style_spider(FILE * fp)
void GnuPlot::SaveStyleSpider(FILE * fp)
{
	fprintf(fp, "set style spiderplot ");
	save_linetype(fp, &Gg.SpiderPlotStyle.lp_properties, TRUE);
	fprintf(fp, "\nset style spiderplot fillstyle ");
	save_fillstyle(fp, &Gg.SpiderPlotStyle.fillstyle);
}

//void save_style_textbox(FILE * fp)
void GnuPlot::SaveStyleTextBox(FILE * fp)
{
	for(int bs = 0; bs < NUM_TEXTBOX_STYLES; bs++) {
		textbox_style * textbox = &Gg.textbox_opts[bs];
		if(textbox->linewidth <= 0)
			continue;
		fprintf(fp, "set style textbox ");
		if(bs > 0)
			fprintf(fp, "%d ", bs);
		fprintf(fp, " %s margins %4.1f, %4.1f", textbox->opaque ? "opaque" : "transparent", textbox->Margin.x, textbox->Margin.y);
		if(textbox->opaque) {
			fprintf(fp, " fc ");
			save_pm3dcolor(fp, &(textbox->fillcolor));
		}
		if(textbox->noborder) {
			fprintf(fp, " noborder");
		}
		else {
			fprintf(fp, " border ");
			save_pm3dcolor(fp, &(textbox->border_color));
		}
		fprintf(fp, " linewidth %4.1f", textbox->linewidth);
		fputs("\n", fp);
	}
}

//void save_position(FILE * fp, const GpPosition * pPos, int ndim, bool offset)
void GnuPlot::SavePosition(FILE * fp, const GpPosition * pPos, int ndim, bool offset)
{
	if(offset) {
		if(pPos->x == 0.0 && pPos->y == 0.0 && pPos->z == 0.0)
			return;
		fprintf(fp, " offset ");
	}
	// Save in time coordinates if appropriate 
	if(pPos->scalex == first_axes) {
		SaveNumOrTimeInput(fp, pPos->x, &AxS[FIRST_X_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", coord_msg[pPos->scalex], pPos->x);
	}
	if(ndim == 1)
		return;
	else
		fprintf(fp, ", ");
	if(pPos->scaley == first_axes || pPos->scalex == polar_axes) {
		if(pPos->scaley != pPos->scalex) 
			fprintf(fp, "first ");
		SaveNumOrTimeInput(fp, pPos->y, &AxS[FIRST_Y_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", (pPos->scaley == pPos->scalex) ? "" : coord_msg[pPos->scaley], pPos->y);
	}
	if(ndim == 2)
		return;
	else
		fprintf(fp, ", ");
	if(pPos->scalez == first_axes) {
		if(pPos->scalez != pPos->scaley) 
			fprintf(fp, "first ");
		SaveNumOrTimeInput(fp, pPos->z, &AxS[FIRST_Z_AXIS]);
	}
	else {
		fprintf(fp, "%s%g", (pPos->scalez == pPos->scaley) ? "" : coord_msg[pPos->scalez], pPos->z);
	}
}

//void save_prange(FILE * fp, GpAxis * this_axis)
void GnuPlot::SavePRange(FILE * fp, const GpAxis & rAx)
{
	bool noextend = FALSE;
	fprintf(fp, "set %srange [ ", axis_name((AXIS_INDEX)rAx.index));
	if(rAx.set_autoscale & AUTOSCALE_MIN) {
		if(rAx.MinConstraint & CONSTRAINT_LOWER) {
			SaveNumOrTimeInput(fp, rAx.min_lb, &rAx);
			fputs(" < ", fp);
		}
		putc('*', fp);
		if(rAx.MinConstraint & CONSTRAINT_UPPER) {
			fputs(" < ", fp);
			SaveNumOrTimeInput(fp, rAx.min_ub, &rAx);
		}
	}
	else {
		SaveNumOrTimeInput(fp, rAx.set_min, &rAx);
	}
	fputs(" : ", fp);
	if(rAx.set_autoscale & AUTOSCALE_MAX) {
		if(rAx.MaxConstraint & CONSTRAINT_LOWER) {
			SaveNumOrTimeInput(fp, rAx.max_lb, &rAx);
			fputs(" < ", fp);
		}
		putc('*', fp);
		if(rAx.MaxConstraint & CONSTRAINT_UPPER) {
			fputs(" < ", fp);
			SaveNumOrTimeInput(fp, rAx.max_ub, &rAx);
		}
	}
	else {
		SaveNumOrTimeInput(fp, rAx.set_max, &rAx);
	}
	if(rAx.index < PARALLEL_AXES)
		fprintf(fp, " ] %sreverse %swriteback", ((rAx.range_flags & RANGE_IS_REVERSED)) ? "" : "no", rAx.range_flags & RANGE_WRITEBACK ? "" : "no");
	else
		fprintf(fp, " ] ");
	if((rAx.set_autoscale & AUTOSCALE_FIXMIN) && (rAx.set_autoscale & AUTOSCALE_FIXMAX)) {
		fprintf(fp, " noextend");
		noextend = TRUE;
	}
	if(rAx.set_autoscale && fp == stderr) {
		// add current (hidden) range as comments 
		fputs("  # (currently [", fp);
		SaveNumOrTimeInput(fp, rAx.min, &rAx);
		putc(':', fp);
		SaveNumOrTimeInput(fp, rAx.max, &rAx);
		fputs("] )\n", fp);
	}
	else
		putc('\n', fp);
	if(!noextend && (fp != stderr)) {
		if(rAx.set_autoscale & (AUTOSCALE_FIXMIN))
			fprintf(fp, "set autoscale %sfixmin\n", axis_name((AXIS_INDEX)rAx.index));
		if(rAx.set_autoscale & AUTOSCALE_FIXMAX)
			fprintf(fp, "set autoscale %sfixmax\n", axis_name((AXIS_INDEX)rAx.index));
	}
}

void save_link(FILE * fp, GpAxis * pAx)
{
	if(pAx->linked_to_primary && pAx->index != -pAx->linked_to_primary->index) {
		fprintf(fp, "set link %s ", axis_name((AXIS_INDEX)pAx->index));
		if(pAx->link_udf->at)
			fprintf(fp, "via %s ", pAx->link_udf->definition);
		if(pAx->linked_to_primary->link_udf->at)
			fprintf(fp, "inverse %s ", pAx->linked_to_primary->link_udf->definition);
		fputs("\n", fp);
	}
}

void save_nonlinear(FILE * fp, GpAxis * pAx)
{
	GpAxis * primary = pAx->linked_to_primary;
	if(primary && pAx->index == -primary->index) {
		fprintf(fp, "set nonlinear %s ", axis_name((AXIS_INDEX)pAx->index));
		if(primary->link_udf->at)
			fprintf(fp, "via %s ", primary->link_udf->definition);
		else
			fprintf(stderr, "[corrupt linkage] ");
		if(pAx->link_udf->at)
			fprintf(fp, "inverse %s ", pAx->link_udf->definition);
		else
			fprintf(stderr, "[corrupt linkage] ");
		fputs("\n", fp);
	}
}

//static void save_zeroaxis(FILE * fp, AXIS_INDEX axis)
void GnuPlot::SaveZeroAxis(FILE * fp, AXIS_INDEX axis)
{
	if(AxS[axis].zeroaxis == NULL) {
		fprintf(fp, "unset %szeroaxis", axis_name(axis));
	}
	else {
		fprintf(fp, "set %szeroaxis", axis_name(axis));
		if(AxS[axis].zeroaxis != &default_axis_zeroaxis)
			save_linetype(fp, AxS[axis].zeroaxis, FALSE);
	}
	putc('\n', fp);
}

void save_fillstyle(FILE * fp, const struct fill_style_type * fs)
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

void save_textcolor(FILE * fp, const struct t_colorspec * tc)
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
			case TC_LT:   if(tc->lt == LT_NODRAW)
				    fprintf(fp, " nodraw");
			    else if(tc->lt == LT_BACKGROUND)
				    fprintf(fp, " bgnd");
			    else
				    fprintf(fp, " lt %d", tc->lt+1);
			    break;
			case TC_LINESTYLE:   fprintf(fp, " linestyle %d", tc->lt);
			    break;
			case TC_Z:    fprintf(fp, " palette z");
			    break;
			case TC_CB:   fprintf(fp, " palette cb %g", tc->value);
			    break;
			case TC_FRAC: fprintf(fp, " palette fraction %4.2f", tc->value);
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

//void save_data_func_style(FILE * fp, const char * which, enum PLOT_STYLE style)
void GnuPlot::SaveDataFuncStyle(FILE * fp, const char * which, enum PLOT_STYLE style)
{
	char * answer = sstrdup(reverse_table_lookup(plotstyle_tbl, style));
	char * idollar = strchr(answer, '$');
	if(idollar) {
		do {
			*idollar = *(idollar+1);
			idollar++;
		} while(*idollar);
	}
	fputs(answer, fp);
	SAlloc::F(answer);
	if(style == FILLEDCURVES) {
		fputs(" ", fp);
		if(sstreq(which, "data") || sstreq(which, "Data"))
			filledcurves_options_tofile(&Gg.filledcurves_opts_data, fp);
		else
			filledcurves_options_tofile(&Gg.filledcurves_opts_func, fp);
	}
	fputc('\n', fp);
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
				int i;
				fputs(" (", fp);
				for(i = 0; i < DASHPATTERN_LENGTH && dt->pattern[i] > 0; i++)
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
	else if(lp->l_type == LT_BACKGROUND)
		fprintf(fp, " lt bgnd");
	else if(lp->l_type == LT_DEFAULT)
		; /* Dont' print anything */
	else if(lp->l_type == LT_AXIS)
		fprintf(fp, " lt 0");
	if(lp->l_type == LT_BLACK && lp->pm3d_color.type == TC_LT)
		fprintf(fp, " lt black");
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
	save_dashtype(fp, lp->d_type, &lp->CustomDashPattern);
	if(show_point) {
		if(lp->PtType == PT_CHARACTER)
			fprintf(fp, " pointtype \"%s\"", lp->p_char);
		else if(lp->PtType == PT_VARIABLE)
			fprintf(fp, " pointtype variable");
		else
			fprintf(fp, " pointtype %d", lp->PtType + 1);
		if(lp->PtSize == PTSZ_VARIABLE)
			fprintf(fp, " pointsize variable");
		else if(lp->PtSize == PTSZ_DEFAULT)
			fprintf(fp, " pointsize default");
		else
			fprintf(fp, " pointsize %.3f", lp->PtSize);
		if(lp->p_interval != 0)
			fprintf(fp, " pointinterval %d", lp->p_interval);
		if(lp->p_number != 0)
			fprintf(fp, " pointnumber %d", lp->p_number);
	}
}

//void save_offsets(FILE * fp, char * lead)
void GnuPlot::SaveOffsets(FILE * fp, char * lead)
{
	fprintf(fp, "%s %s%g, %s%g, %s%g, %s%g\n", lead,
	    Gr.LOff.scalex == graph ? "graph " : "", Gr.LOff.x,
	    Gr.ROff.scalex == graph ? "graph " : "", Gr.ROff.x,
	    Gr.TOff.scaley == graph ? "graph " : "", Gr.TOff.y,
	    Gr.BOff.scaley == graph ? "graph " : "", Gr.BOff.y);
}

//void save_bars(FILE * fp)
void GnuPlot::SaveBars(FILE * fp)
{
	if(Gr.BarSize == 0.0) {
		fprintf(fp, "unset errorbars\n");
	}
	else {
		fprintf(fp, "set errorbars %s", (Gr.BarLayer == LAYER_BACK) ? "back" : "front");
		if(Gr.BarSize > 0.0)
			fprintf(fp, " %f ", Gr.BarSize);
		else
			fprintf(fp, " fullwidth ");
		if(Gr.BarLp.flags & LP_ERRORBAR_SET)
			save_linetype(fp, &Gr.BarLp, FALSE);
		fputs("\n", fp);
	}
}

//void save_histogram_opts(FILE * fp)
void GnuPlot::SaveHistogramOpts(FILE * fp)
{
	switch(Gg.histogram_opts.type) {
		default:
		case HT_CLUSTERED: fprintf(fp, "clustered gap %d ", Gg.histogram_opts.gap); break;
		case HT_ERRORBARS: fprintf(fp, "errorbars gap %d lw %g", Gg.histogram_opts.gap, Gg.histogram_opts.bar_lw); break;
		case HT_STACKED_IN_LAYERS: fprintf(fp, "rowstacked "); break;
		case HT_STACKED_IN_TOWERS: fprintf(fp, "columnstacked "); break;
	}
	if(fp == stderr)
		fprintf(fp, "\n\t\t");
	fprintf(fp, "title");
	save_textcolor(fp, &Gg.histogram_opts.title.textcolor);
	if(Gg.histogram_opts.title.font)
		fprintf(fp, " font \"%s\" ", Gg.histogram_opts.title.font);
	SavePosition(fp, &Gg.histogram_opts.title.offset, 2, TRUE);
	fprintf(fp, "\n");
}

//void save_pixmaps(FILE * fp)
void GnuPlot::SavePixmaps(FILE * fp)
{
	for(t_pixmap * pixmap = Gg.P_PixmapListHead; pixmap; pixmap = pixmap->next) {
		if(pixmap->filename)
			fprintf(fp, "set pixmap %d '%s' # (%d x %d pixmap)\n", pixmap->tag, pixmap->filename, pixmap->ncols, pixmap->nrows);
		if(pixmap->colormapname)
			fprintf(fp, "set pixmap %d colormap %s # (%d x %d pixmap)\n", pixmap->tag, pixmap->colormapname, pixmap->ncols, pixmap->nrows);
		fprintf(fp, "set pixmap %d at ", pixmap->tag);
		SavePosition(fp, &pixmap->pin, 3, FALSE);
		fprintf(fp, "  size ");
		SavePosition(fp, &pixmap->extent, 2, FALSE);
		fprintf(fp, " %s %s\n", pixmap->layer == LAYER_FRONT ? "front" : pixmap->layer == LAYER_BACK ? "back" : "behind", pixmap->center ? "center" : "");
	}
}
//
// Save/show rectangle <tag> (0 means show all) 
//
//void save_object(FILE * fp, int tag)
void GnuPlot::SaveObject(FILE * fp, int tag)
{
	t_object * this_object;
	t_rectangle * this_rect;
	t_circle * this_circle;
	t_ellipse * this_ellipse;
	bool showed = FALSE;
	for(this_object = Gg.P_FirstObject; this_object; this_object = this_object->next) {
		if((this_object->object_type == OBJ_RECTANGLE) && (tag == 0 || tag == this_object->tag)) {
			this_rect = &this_object->o.rectangle;
			showed = TRUE;
			fprintf(fp, "%sobject %2d rect ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			if(this_rect->type == 1) {
				fprintf(fp, "center ");
				SavePosition(fp, &this_rect->center, 2, FALSE);
				fprintf(fp, " size ");
				SavePosition(fp, &this_rect->extent, 2, FALSE);
			}
			else {
				fprintf(fp, "from ");
				SavePosition(fp, &this_rect->bl, 2, FALSE);
				fprintf(fp, " to ");
				SavePosition(fp, &this_rect->tr, 2, FALSE);
			}
		}
		else if((this_object->object_type == OBJ_CIRCLE) && (tag == 0 || tag == this_object->tag)) {
			GpPosition * e = &this_object->o.circle.extent;
			this_circle = &this_object->o.circle;
			showed = TRUE;
			fprintf(fp, "%sobject %2d circle ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			fprintf(fp, "center ");
			SavePosition(fp, &this_circle->center, 3, FALSE);
			fprintf(fp, " size ");
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, " arc [%g:%g] ", this_circle->ArcR.low, this_circle->ArcR.upp);
			fprintf(fp, this_circle->wedge ? "wedge " : "nowedge");
		}
		else if((this_object->object_type == OBJ_ELLIPSE) && (tag == 0 || tag == this_object->tag)) {
			GpPosition * e = &this_object->o.ellipse.extent;
			this_ellipse = &this_object->o.ellipse;
			showed = TRUE;
			fprintf(fp, "%sobject %2d ellipse ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			fprintf(fp, "center ");
			SavePosition(fp, &this_ellipse->center, 3, FALSE);
			fprintf(fp, " size ");
			fprintf(fp, "%s%g", e->scalex == first_axes ? "" : coord_msg[e->scalex], e->x);
			fprintf(fp, ", %s%g", e->scaley == e->scalex ? "" : coord_msg[e->scaley], e->y);
			fprintf(fp, "  angle %g", this_ellipse->orientation);
			fputs(" units ", fp);
			switch(this_ellipse->type) {
				case ELLIPSEAXES_XY: fputs("xy", fp); break;
				case ELLIPSEAXES_XX: fputs("xx", fp); break;
				case ELLIPSEAXES_YY: fputs("yy", fp); break;
			}
		}
		else if((this_object->object_type == OBJ_POLYGON) && (tag == 0 || tag == this_object->tag)) {
			t_polygon * this_polygon = &this_object->o.polygon;
			int nv;
			showed = TRUE;
			fprintf(fp, "%sobject %2d polygon ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			if(this_polygon->vertex) {
				fprintf(fp, "from ");
				SavePosition(fp, &this_polygon->vertex[0], 3, FALSE);
			}
			for(nv = 1; nv < this_polygon->type; nv++) {
				fprintf(fp, (fp==stderr) ? "\n\t\t\t    to " : " to ");
				SavePosition(fp, &this_polygon->vertex[nv], 3, FALSE);
			}
		}
		// Properties common to all objects 
		if(tag == 0 || tag == this_object->tag) {
			fprintf(fp, "\n%sobject %2d ", (fp==stderr) ? "\t" : "set ", this_object->tag);
			fprintf(fp, "%s ", this_object->layer == LAYER_FRONT ? "front" : this_object->layer == LAYER_DEPTHORDER ? "depthorder" : this_object->layer == LAYER_BEHIND ? "behind" : "back");
			if(this_object->clip == OBJ_NOCLIP)
				fputs("noclip ", fp);
			else
				fputs("clip ", fp);
			if(this_object->lp_properties.l_width)
				fprintf(fp, "lw %.1f ", this_object->lp_properties.l_width);
			if(this_object->lp_properties.d_type)
				save_dashtype(fp, this_object->lp_properties.d_type, &this_object->lp_properties.CustomDashPattern);
			fprintf(fp, " fc ");
			if(this_object->lp_properties.l_type == LT_DEFAULT)
				fprintf(fp, "default");
			else
				save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
			fprintf(fp, " fillstyle ");
			save_fillstyle(fp, &this_object->fillstyle);
		}
	}
	if(tag > 0 && !showed)
		IntErrorCurToken("object not found");
}
//
// Save/show special polygon objects created by "set wall" 
//
//void save_walls(FILE * fp)
void GnuPlot::SaveWalls(FILE * fp)
{
	static const char * wall_name[5] = {"y0", "x0", "y1", "x1", "z0"};
	for(int i = 0; i < 5; i++) {
		t_object * this_object = &Gg.GridWall[i];
		if(this_object->layer == LAYER_FRONTBACK) {
			fprintf(fp, "set wall %s ", wall_name[i]);
			fprintf(fp, " fc ");
			save_pm3dcolor(fp, &this_object->lp_properties.pm3d_color);
			fprintf(fp, " fillstyle ");
			save_fillstyle(fp, &this_object->fillstyle);
		}
	}
}
