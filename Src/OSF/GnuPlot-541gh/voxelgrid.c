// GNUPLOT - voxelgrid.c 
// Copyright Ethan A Merritt 2019
//
/*
 * This file implements a 3D grid of NxNxN evenly spaced voxels.
 * The command
 *	set vgrid size N
 * frees any previous grid, allocates a new one, and initializes it to zero.
 * The grid range can be set explicitly using
 *	set vxrange [vxmin:vxmax]
 *	set vyrange [vymin:vymax]
 *	set vzrange [vzmin:vzmax]
 * otherwise it inherits the xrange, yrange, and zrange limits present at the
 * time of the first "vclear", "vfill", or "voxel() = " command.
 *
 * Grid voxels can be filled individually using the command
 *      voxel(x,y,z) = FOO
 * or filled in the neighborhood of a data point using the command
 *      vfill $FILE using x:y:z:radius:(function)
 * In this case the nearest grid point and all other grid points within
 * a sphere centered about [x,y,z] are incremented by
 *	voxel(vx,vy,vz) += function( sqrt((x-vx)**2 + (y-vy)**2 + (z-vz)**2) )
 * Once loaded the grid can either be referenced by splot commands, e.g.
 *      splot $FILE using x:y:z:(voxel(x,y,z)) with points lc palette
 * or plotted using new splot options
 *      splot $GRID with {dots|points}
 *      splot $GRID with isosurface level <value>
 *
 * The grid can be cleared by
 *	vclear
 * or deallocated by
 *      unset vgrid
 */
#include <gnuplot.h>
#pragma hdrstop

#ifdef VOXEL_GRID_SUPPORT
//
//void init_voxelsupport()
void GnuPlot::InitVoxelSupport()
{
	// Make sure there are user variables that can be used as a parameter
	// to the function in the 5th spec of "vfill".
	// Scripts can test if (exists("VoxelDistance")) to check for voxel support.
	_VG.P_UdvVoxelDistance = Ev.AddUdvByName("VoxelDistance");
	_VG.P_UdvVoxelDistance->udv_value.Type = CMPLX;
	Gcomplex(&_VG.P_UdvVoxelDistance->udv_value, 0.0, 0.0);
	_VG.P_UdvGridDistance = Ev.AddUdvByName("GridDistance");
	_VG.P_UdvGridDistance->udv_value.Type = CMPLX;
	Gcomplex(&_VG.P_UdvGridDistance->udv_value, 0.0, 0.0);
	// default state of other voxel-related structures 
	_VG.IsoSurfaceOptions.inside_offset = 1; // inside color = outside + 1 
	_VG.IsoSurfaceOptions.tessellation = 0; // mixed triangles + quadrangles 
}
// 
// vx vy vz ranges must be established before the grid can be used
// 
//void check_grid_ranges()
void GnuPlot::CheckGridRanges()
{
	if(_VG.P_CurrentVGrid == NULL)
		IntError(NO_CARET, "vgrid must be set before use");
	else {
		if(isnan(_VG.P_CurrentVGrid->vxmin) || isnan(_VG.P_CurrentVGrid->vxmax)) {
			if((AxS[FIRST_X_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
				_VG.P_CurrentVGrid->vxmin = AxS[FIRST_X_AXIS].set_min;
				_VG.P_CurrentVGrid->vxmax = AxS[FIRST_X_AXIS].set_max;
			}
			else
				IntError(NO_CARET, "grid limits must be set before use");
		}
		if(isnan(_VG.P_CurrentVGrid->vymin) || isnan(_VG.P_CurrentVGrid->vymax)) {
			if((AxS[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
				_VG.P_CurrentVGrid->vymin = AxS[FIRST_Y_AXIS].set_min;
				_VG.P_CurrentVGrid->vymax = AxS[FIRST_Y_AXIS].set_max;
			}
			else
				IntError(NO_CARET, "grid limits must be set before use");
		}
		if(isnan(_VG.P_CurrentVGrid->vzmin) || isnan(_VG.P_CurrentVGrid->vzmax)) {
			if((AxS[FIRST_Z_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
				_VG.P_CurrentVGrid->vzmin = AxS[FIRST_Z_AXIS].set_min;
				_VG.P_CurrentVGrid->vzmax = AxS[FIRST_Z_AXIS].set_max;
			}
			else
				IntError(NO_CARET, "grid limits must be set before use");
		}
		_VG.P_CurrentVGrid->vxdelta = (_VG.P_CurrentVGrid->vxmax - _VG.P_CurrentVGrid->vxmin) / (_VG.P_CurrentVGrid->size - 1);
		_VG.P_CurrentVGrid->vydelta = (_VG.P_CurrentVGrid->vymax - _VG.P_CurrentVGrid->vymin) / (_VG.P_CurrentVGrid->size - 1);
		_VG.P_CurrentVGrid->vzdelta = (_VG.P_CurrentVGrid->vzmax - _VG.P_CurrentVGrid->vzmin) / (_VG.P_CurrentVGrid->size - 1);
	}
}
// 
// Initialize vgrid array
// set vgrid <name> {size <N>}
//   - retrieve existing voxel grid or create a new one
//   - initialize to N (defaults to N=100 for a new grid)
//   - set _VG.P_CurrentVGrid to this grid.
// 
//void set_vgrid()
void GnuPlot::SetVGrid()
{
	udvt_entry * grid = NULL;
	int new_size = 100; // default size 
	char * name;
	Pgm.Shift();
	if(Pgm.EndOfCommand() || !Pgm.IsLetter(Pgm.GetCurTokenIdx()+1))
		IntErrorCurToken("syntax: set vgrid $<gridname> {size N}");
	// Create or recycle a datablock with the requested name 
	name = Pgm.ParseDatablockName();
	grid = Ev.AddUdvByName(name);
	if(grid->udv_value.Type == VOXELGRID) {
		// Keep size of existing grid 
		new_size = grid->udv_value.v.vgrid->size;
		_VG.P_CurrentVGrid = grid->udv_value.v.vgrid;
	}
	else {
		// Note: The only other variable type that starts with a $ is DATABLOCK 
		grid->udv_value.Destroy();
		_VG.P_CurrentVGrid = (VGrid *)SAlloc::M(sizeof(VGrid));
		memzero(_VG.P_CurrentVGrid, sizeof(VGrid));
		_VG.P_CurrentVGrid->vxmin = fgetnan();
		_VG.P_CurrentVGrid->vxmax = fgetnan();
		_VG.P_CurrentVGrid->vymin = fgetnan();
		_VG.P_CurrentVGrid->vymax = fgetnan();
		_VG.P_CurrentVGrid->vzmin = fgetnan();
		_VG.P_CurrentVGrid->vzmax = fgetnan();
		grid->udv_value.v.vgrid = _VG.P_CurrentVGrid;
		grid->udv_value.Type = VOXELGRID;
	}
	if(Pgm.EqualsCur("size")) {
		Pgm.Shift();
		new_size = IntExpression();
	}
	// FIXME: arbitrary limit to reduce chance of memory exhaustion 
	if(new_size < 10 || new_size > 256)
		IntError(NO_CARET, "vgrid size must be an integer between 10 and 256");
	// Initialize storage for new voxel grid 
	if(_VG.P_CurrentVGrid->size != new_size) {
		_VG.P_CurrentVGrid->size = new_size;
		_VG.P_CurrentVGrid->vdata = (t_voxel *)SAlloc::R(_VG.P_CurrentVGrid->vdata, new_size*new_size*new_size*sizeof(t_voxel));
		memzero(_VG.P_CurrentVGrid->vdata, new_size*new_size*new_size*sizeof(t_voxel));
	}
}
//
// set vxrange [min:max]
//
//void set_vgrid_range()
void GnuPlot::SetVGridRange()
{
	double gridmin;
	double gridmax;
	int    save_token = Pgm.Shift();
	if(!_VG.P_CurrentVGrid)
		IntError(NO_CARET, "no voxel grid is active");
	if(!Pgm.EqualsCur("["))
		return;
	Pgm.Shift();
	gridmin = RealExpression();
	if(!Pgm.EqualsCur(":"))
		return;
	Pgm.Shift();
	gridmax = RealExpression();
	if(!Pgm.EqualsCur("]"))
		return;
	Pgm.Shift();
	if(Pgm.AlmostEquals(save_token, "vxr$ange")) {
		_VG.P_CurrentVGrid->vxmin = gridmin;
		_VG.P_CurrentVGrid->vxmax = gridmax;
	}
	if(Pgm.AlmostEquals(save_token, "vyr$ange")) {
		_VG.P_CurrentVGrid->vymin = gridmin;
		_VG.P_CurrentVGrid->vymax = gridmax;
	}
	if(Pgm.AlmostEquals(save_token, "vzr$ange")) {
		_VG.P_CurrentVGrid->vzmin = gridmin;
		_VG.P_CurrentVGrid->vzmax = gridmax;
	}
}
//
// show state of all voxel grids
//
//void show_vgrid()
void GnuPlot::ShowVGrid()
{
	for(udvt_entry * udv = Ev.P_FirstUdv; udv; udv = udv->next_udv) {
		if(udv->udv_value.Type == VOXELGRID) {
			VGrid * vgrid = udv->udv_value.v.vgrid;
			fprintf(stderr, "\t%s:", udv->udv_name);
			if(vgrid == _VG.P_CurrentVGrid)
				fprintf(stderr, "\t(active)");
			fprintf(stderr, "\n");
			fprintf(stderr, "\t\tsize %d X %d X %d\n", vgrid->size, vgrid->size, vgrid->size);
			if(isnan(vgrid->vxmin) || isnan(vgrid->vxmax) || isnan(vgrid->vymin) ||  isnan(vgrid->vymax) || isnan(vgrid->vzmin) || isnan(vgrid->vzmax)) {
				fprintf(stderr, "\t\tgrid ranges not set\n");
				continue;
			}
			fprintf(stderr, "\t\tvxrange [%g:%g]  vyrange[%g:%g]  vzrange[%g:%g]\n", vgrid->vxmin, vgrid->vxmax, vgrid->vymin, vgrid->vymax, vgrid->vzmin, vgrid->vzmax);
			vgrid_stats(vgrid);
			fprintf(stderr, "\t\tnon-zero voxel values:  min %.2g max %.2g  mean %.2g stddev %.2g\n", vgrid->min_value, vgrid->max_value, vgrid->mean_value, vgrid->stddev);
			fprintf(stderr, "\t\tnumber of zero voxels:  %d   (%.2f%%)\n", vgrid->nzero, 100. * (double)vgrid->nzero / (vgrid->size*vgrid->size*vgrid->size));
		}
	}
}
/*
 * run through the whole grid
 * accumulate min/max, mean, and standard deviation of non-zero voxels
 * TODO: median
 * TODO: only count voxels in range on x y and z
 */
void vgrid_stats(VGrid * vgrid)
{
	double min = VERYLARGE;
	double max = -VERYLARGE;
	double sum = 0;
	int nzero = 0;
	t_voxel * voxel;
	int N = vgrid->size;
	int i;
	// bookkeeping for standard deviation 
	double num = 0;
	double delta = 0;
	double delta2 = 0;
	double mean = 0;
	double mean2 = 0;
	for(voxel = vgrid->vdata, i = 0; i < N*N*N; voxel++, i++) {
		if(*voxel == 0.0f)
			nzero++;
		else {
			sum += *voxel;
			SETMIN(min, *voxel);
			SETMAX(max, *voxel);
			// standard deviation 
			num += 1.0;
			delta = *voxel - mean;
			mean += delta/num;
			delta2 = *voxel - mean;
			mean2 += delta * delta2;
		}
	}
	vgrid->min_value = min;
	vgrid->max_value = max;
	vgrid->nzero = nzero;
	vgrid->sum = sum;
	if(num < 2) {
		vgrid->mean_value = vgrid->stddev = fgetnan();
	}
	else {
		vgrid->mean_value = sum / (double)(N*N*N - nzero);
		vgrid->stddev = sqrt(mean2 / (num - 1.0));
	}
	/* all zeros */
	if(nzero == N*N*N) {
		vgrid->min_value = 0.0;
		vgrid->max_value = 0.0;
	}
}

//udvt_entry * get_vgrid_by_name(const char * name)
udvt_entry * GnuPlot::GetVGridByName(const char * pName)
{
	udvt_entry * vgrid = Ev.GetUdvByName(pName);
	return (!vgrid || vgrid->udv_value.Type != VOXELGRID) ? NULL : vgrid;
}
// 
// initialize content of voxel grid to all zero
// 
//void vclear_command()
void GnuPlot::VClearCommand()
{
	VGrid * vgrid = _VG.P_CurrentVGrid;
	Pgm.Shift();
	if(!Pgm.EndOfCommand() && Pgm.EqualsCur("$")) {
		char * name = Pgm.ParseDatablockName();
		udvt_entry * vgrid_udv = GetVGridByName(name);
		if(!vgrid_udv)
			IntErrorCurToken("no such voxel grid");
		vgrid = vgrid_udv->udv_value.v.vgrid;
	}
	if(vgrid && vgrid->size && vgrid->vdata) {
		int size = vgrid->size;
		memzero(vgrid->vdata, size*size*size * sizeof(t_voxel));
	}
}
// 
// deallocate storage for a voxel grid
// 
void GnuPlot::GpVoxelGrid::FreeGrid(udvt_entry * pGrid)
{
	if(pGrid->udv_value.Type == VOXELGRID) {
		SAlloc::F(pGrid->udv_value.v.vgrid->vdata);
		SAlloc::F(pGrid->udv_value.v.vgrid);
		if(pGrid->udv_value.v.vgrid == P_CurrentVGrid)
			P_CurrentVGrid = NULL;
		pGrid->udv_value.v.vgrid = NULL;
		pGrid->udv_value.SetNotDefined();
	}
}
//
// 'unset $vgrid' command
//
void GnuPlot::UnsetVGrid()
{
	if(Pgm.EndOfCommand() || !Pgm.EqualsCur("$"))
		IntErrorCurToken("syntax: unset vgrid $<gridname>");
	else {
		// Look for a datablock with the requested name 
		char * name = Pgm.ParseDatablockName();
		udvt_entry * grid = GetVGridByName(name);
		if(!grid)
			IntErrorCurToken("no such vgrid");
		else
			_VG.FreeGrid(grid);
	}
}
//
// "set isosurface {triangles|mixed}"
//
//void set_isosurface()
void GnuPlot::SetIsoSurface()
{
	while(!Pgm.EndOfCommand()) {
		Pgm.Shift();
		if(Pgm.AlmostEqualsCur("triang$les")) {
			Pgm.Shift();
			_VG.IsoSurfaceOptions.tessellation = 1;
		}
		else if(Pgm.AlmostEqualsCur("mix$ed")) {
			Pgm.Shift();
			_VG.IsoSurfaceOptions.tessellation = 0;
		}
		else if(Pgm.AlmostEqualsCur("inside$color")) {
			Pgm.Shift();
			if(Pgm.EndOfCommand())
				_VG.IsoSurfaceOptions.inside_offset = 1;
			else
				_VG.IsoSurfaceOptions.inside_offset = IntExpression();
		}
		else if(Pgm.AlmostEqualsCur("noin$sidecolor")) {
			Pgm.Shift();
			_VG.IsoSurfaceOptions.inside_offset = 0;
		}
		else
			IntErrorCurToken("unrecognized option");
	}
}

//void show_isosurface()
void GnuPlot::ShowIsoSurface()
{
	Pgm.Shift();
	fprintf(stderr, "\tisosurfaces will use %s\n", _VG.IsoSurfaceOptions.tessellation ? "triangles only" : "a mixture of triangles and quadrangles");
	fprintf(stderr, "\tinside surface linetype offset by %d\n", _VG.IsoSurfaceOptions.inside_offset);
}
// 
// voxel(x,y,z) = expr()
// 
//void voxel_command()
void GnuPlot::VoxelCommand()
{
	double vx, vy, vz;
	t_voxel * voxel;
	CheckGridRanges();
	Pgm.Shift();
	if(!Pgm.EqualsCurShift("("))
		IntError(Pgm.GetPrevTokenIdx(), "syntax: voxel(x,y,z) = newvalue");
	vx = RealExpression();
	if(!Pgm.EqualsCurShift(","))
		IntError(Pgm.GetPrevTokenIdx(), "syntax: voxel(x,y,z) = newvalue");
	vy = RealExpression();
	if(!Pgm.EqualsCurShift(","))
		IntError(Pgm.GetPrevTokenIdx(), "syntax: voxel(x,y,z) = newvalue");
	vz = RealExpression();
	if(!Pgm.EqualsCurShift(")"))
		IntError(Pgm.GetPrevTokenIdx(), "syntax: voxel(x,y,z) = newvalue");
	if(!Pgm.EqualsCurShift("="))
		IntError(Pgm.GetPrevTokenIdx(), "syntax: voxel(x,y,z) = newvalue");
	if(vx < _VG.P_CurrentVGrid->vxmin || vx > _VG.P_CurrentVGrid->vxmax || vy < _VG.P_CurrentVGrid->vymin || vy > _VG.P_CurrentVGrid->vymax || vz < _VG.P_CurrentVGrid->vzmin || vz > _VG.P_CurrentVGrid->vzmax) {
		IntWarn(NO_CARET, "voxel out of range");
		RealExpression();
	}
	else {
		int ivx = fceili((vx - _VG.P_CurrentVGrid->vxmin) / _VG.P_CurrentVGrid->vxdelta);
		int ivy = fceili((vy - _VG.P_CurrentVGrid->vymin) / _VG.P_CurrentVGrid->vydelta);
		int ivz = fceili((vz - _VG.P_CurrentVGrid->vzmin) / _VG.P_CurrentVGrid->vzdelta);
		int N = _VG.P_CurrentVGrid->size;
		FPRINTF((stderr, "\tvgrid array index = %d\n",  ivx + ivy * N + ivz * N*N));
		voxel = &_VG.P_CurrentVGrid->vdata[ ivx + ivy * N + ivz * N*N ];
		*voxel = static_cast<t_voxel>(RealExpression());
	}
}
// 
// internal look-up function voxel(x,y,z)
// 
t_voxel GnuPlot::GpVoxelGrid::Voxel(double vx, double vy, double vz) const
{
	if(!P_CurrentVGrid)
		return static_cast<t_voxel>(fgetnan());
	else if(vx < P_CurrentVGrid->vxmin || vx > P_CurrentVGrid->vxmax || vy < P_CurrentVGrid->vymin || vy > P_CurrentVGrid->vymax || vz < P_CurrentVGrid->vzmin || vz > P_CurrentVGrid->vzmax)
		return static_cast<t_voxel>(fgetnan());
	else {
		int ivx = fceili((vx - P_CurrentVGrid->vxmin) / P_CurrentVGrid->vxdelta);
		int ivy = fceili((vy - P_CurrentVGrid->vymin) / P_CurrentVGrid->vydelta);
		int ivz = fceili((vz - P_CurrentVGrid->vzmin) / P_CurrentVGrid->vzdelta);
		int N = P_CurrentVGrid->size;
		return P_CurrentVGrid->vdata[ivx + ivy * N + ivz * N*N];
	}
}
// 
// user-callable retrieval function voxel(x,y,z)
// 
//void f_voxel(union argument * /*arg*/)
void GnuPlot::F_Voxel(union argument * x)
{
	GpValue a;
	double vz = Real(Pop(&a));
	double vy = Real(Pop(&a));
	double vx = Real(Pop(&a));
	if(!_VG.P_CurrentVGrid)
		IntError(NO_CARET, "no active voxel grid");
	if(vx < _VG.P_CurrentVGrid->vxmin || vx > _VG.P_CurrentVGrid->vxmax || vy < _VG.P_CurrentVGrid->vymin || vy > _VG.P_CurrentVGrid->vymax || vz < _VG.P_CurrentVGrid->vzmin || vz > _VG.P_CurrentVGrid->vzmax)
		Push(&(Ev.P_UdvNaN->udv_value));
	else
		Push(Gcomplex(&a, _VG.Voxel(vx, vy, vz), 0.0) );
}
/*
 * "vfill" works very much like "plot" in that it reads from an input stream
 * of data points specified by the same "using" "skip" "every" and other keyword
 * syntax shared with the "plot" "splot" and "stats" commands.
 * Basic example:
 *      vfill $FILE using x:y:z:radius:(function())
 * However instead of creating a plot immediately, vfill modifies the content
 * of a voxel grid by incrementing the value of each voxel within a radius.
 *	voxel(vx,vy,vz) += function( distance([vx,vy,vz], [x,y,z]) )
 *
 * vfill shares the routines df_open df_readline ... with the plot and splot
 * commands.
 *
 * "vgfill" is exactly the same except that it uses grid coordinates rather
 * than user coordinates.  If the user coordinate system is isotropic
 * (e.g. "set view equal xyz") then vfill and vgfill are identical except
 * possibly for a linear scale factor.
 */
void GnuPlot::VFillCommand()
{
	bool gridcoordinates = Pgm.EqualsCurShift("vgfill");
	VFill(_VG.P_CurrentVGrid->vdata, gridcoordinates);
}

void GnuPlot::VFill(t_voxel * pGrid, bool gridCoordinates)
{
	int plot_num = 0;
	curve_points dummy_plot;
	curve_points * this_plot = &dummy_plot;
	GpValue original_value_sample_var;
	char * name_str;
	CheckGridRanges();
	//
	// This part is modelled on eval_plots()
	//
	_Pb.plot_iterator = CheckForIteration();
	while(TRUE) {
		int sample_range_token;
		int start_token = Pgm.GetCurTokenIdx();
		int specs;
		// Forgive trailing comma on a multi-element plot command 
		if(Pgm.EndOfCommand()) {
			if(plot_num == 0)
				IntErrorCurToken("data input source expected");
			break;
		}
		plot_num++;
		// Check for a sampling range 
		if(Pgm.EqualsCur("sample") && Pgm.EqualsNext("["))
			Pgm.Shift();
		sample_range_token = ParseRange(SAMPLE_AXIS);
		if(sample_range_token != 0)
			AxS[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;
		// FIXME: allow replacement of dummy variable? 
		name_str = StringOrExpress(NULL);
		if(!name_str)
			IntErrorCurToken("no input data source");
		if(sstreq(name_str, "@@"))
			IntError(Pgm.GetPrevTokenIdx(), "filling from array not supported");
		if(sample_range_token !=0 && *name_str != '+')
			IntWarn(sample_range_token, "Ignoring sample range in non-sampled data plot");
		// Dummy up a plot structure so that we can share code with plot command 
		memzero(this_plot, sizeof(curve_points));
		// FIXME:  what exactly do we need to fill in? 
		this_plot->plot_type = DATA;
		this_plot->noautoscale = TRUE;
		specs = DfOpen(name_str, 5, this_plot); // Fixed number of input columns x:y:z:radius:(density_function) 
		// We will invoke density_function in modify_voxels rather than df_readline 
		if(_Df.UseSpec[4].at == NULL)
			IntError(NO_CARET, "5th user spec to vfill must be an expression");
		else {
			free_at(_VG.P_DensityFunction);
			_VG.P_DensityFunction = _Df.UseSpec[4].at;
			_Df.UseSpec[4].at = NULL;
			_Df.UseSpec[4].column = 0;
		}
		// Initialize user variables used by density_function() 
		Gcomplex(&_VG.P_UdvVoxelDistance->udv_value, 0.0, 0.0);
		Gcomplex(&_VG.P_UdvGridDistance->udv_value, 0.0, 0.0);
		// Store a pointer to the named variable used for sampling 
		// Save prior value of sample variables so we can restore them later 
		this_plot->sample_var = Ev.AddUdvByName(_Pb.c_dummy_var[0]);
		original_value_sample_var = this_plot->sample_var->udv_value;
		this_plot->sample_var->udv_value.SetNotDefined();
		// We don't support any further options 
		if(Pgm.AlmostEqualsCur("w$ith"))
			IntErrorCurToken("vfill does not support 'with' options");
		// This part is modelled on get_data().
		// However we process each point as we come to it.
		if(_Df.df_no_use_specs == 5) {
			int j;
			double v[MAXDATACOLS];
			memzero(v, sizeof(v));
			// Initialize stats 
			int ngood = 0;
			_VG.NVoxelsModified = 0;
			fprintf(stderr, "vfill from %s :\n", name_str);
			// If the user has set an explicit locale for numeric input, apply it 
			// here so that it affects data fields read from the input file.      
			set_numeric_locale();
			// Initial state 
			_Df.df_warn_on_missing_columnheader = true;
			while((j = DfReadLine(v, 5)) != DF_EOF) {
				switch(j) {
					case 0:
					    DfClose();
					    IntError(this_plot->token, "Bad data on line %d of file %s", _Df.df_line_number, NZOR(_Df.df_filename, ""));
					    continue;
					case DF_UNDEFINED:
					case DF_MISSING:
					    continue;
					case DF_FIRST_BLANK:
					case DF_SECOND_BLANK:
					    continue;
					case DF_COLUMN_HEADERS:
					case DF_FOUND_KEY_TITLE:
					case DF_KEY_TITLE_MISSING:
					    continue;
					default:
					    break; /* Not continue! */
				}
				// Ignore out of range points 
				// FIXME: probably should be range + radius! 
				if(v[0] < _VG.P_CurrentVGrid->vxmin || _VG.P_CurrentVGrid->vxmax < v[0])
					continue;
				if(v[1] < _VG.P_CurrentVGrid->vymin || _VG.P_CurrentVGrid->vymax < v[1])
					continue;
				if(v[2] < _VG.P_CurrentVGrid->vzmin || _VG.P_CurrentVGrid->vzmax < v[2])
					continue;
				// Now we know for sure we will use the data point 
				ngood++;
				// At this point get_data() would interpret the contents of v[]
				// according to the current plot style and store it.
				// vfill() has a fixed interpretation of v[] and will use it to
				// modify the current voxel grid.
				ModifyVoxels(pGrid, v[0], v[1], v[2], v[3], _VG.P_DensityFunction, gridCoordinates);
			} // End of loop over input data points 
			DfClose();
			// We are finished reading user input; return to C locale for internal use 
			reset_numeric_locale();
			if(ngood == 0) {
				if(!forever_iteration(_Pb.plot_iterator))
					IntWarn(NO_CARET, "Skipping data file with no valid points");
				this_plot->plot_type = NODATA;
			}
			// print some basic stats 
			fprintf(stderr, "\tnumber of points input:    %8d\n", ngood);
			fprintf(stderr, "\tnumber of voxels modified: %8d\n", _VG.NVoxelsModified);
		}
		else if(specs < 0) {
			this_plot->plot_type = NODATA;
		}
		else {
			IntError(NO_CARET, "vfill requires exactly 5 using specs x:y:z:radius:(func)");
		}
		// restore original value of sample variables 
		this_plot->sample_var->udv_value = original_value_sample_var;
		// Iterate-over-plot mechanism 
		if(empty_iteration(_Pb.plot_iterator) && this_plot)
			this_plot->plot_type = NODATA;
		if(forever_iteration(_Pb.plot_iterator) && (this_plot->plot_type == NODATA)) {
			; // nothing to do here 
		}
		else if(NextIteration(_Pb.plot_iterator)) {
			Pgm.SetTokenIdx(start_token);
			continue;
		}
		_Pb.plot_iterator = cleanup_iteration(_Pb.plot_iterator);
		if(Pgm.EqualsCur(",")) {
			Pgm.Shift();
			_Pb.plot_iterator = CheckForIteration();
		}
		else
			break;
	}
	if(plot_num == 0)
		IntErrorCurToken("no data to plot");
	Pgm.plot_token = -1;
}
// 
// This is called by vfill for every data point.
// It modifies all voxels within a specified radius of the point coordinates.
// There are two modes of operation
//   TRUE (vgfill)	- radius and distance are calculated in grid coordinates
//   FALSE (vfill)	- radius and distance are calculated in user coordinates
// Calculation in user coordinates becomes increasingly problematic as the
// nominal grid spacing along x, y, and z diverges due to unequal
// vxrange, vyrange, vzrange.
// 
void GnuPlot::ModifyVoxels(t_voxel * pGrid, double x, double y, double z, double radius, at_type * pDensityFunction, bool gridcoordinates)
{
	GpValue a;
	int ix, iy, iz;
	int nvx, nvy, nvz;
	bool save_fpe_trap;
	if(x < _VG.P_CurrentVGrid->vxmin || x > _VG.P_CurrentVGrid->vxmax || y < _VG.P_CurrentVGrid->vymin || y > _VG.P_CurrentVGrid->vymax || z < _VG.P_CurrentVGrid->vzmin || z > _VG.P_CurrentVGrid->vzmax)
		IntError(NO_CARET, "voxel out of range");
	const int N = _VG.P_CurrentVGrid->size;
	// gvx, gvy, gvz are the fractional indicies of this point 
	// ivx, ivy, ivz are the indices of the nearest grid point 
	const double gvx = (x - _VG.P_CurrentVGrid->vxmin) / _VG.P_CurrentVGrid->vxdelta;
	const double gvy = (y - _VG.P_CurrentVGrid->vymin) / _VG.P_CurrentVGrid->vydelta;
	const double gvz = (z - _VG.P_CurrentVGrid->vzmin) / _VG.P_CurrentVGrid->vzdelta;
	const int ivx = fceili(gvx);
	const int ivy = fceili(gvy);
	const int ivz = fceili(gvz);
	// nvx, nvy, nvz are the number of grid points within radius 
	if(gridcoordinates) {
		// Grid coordinates are isotropic 
		nvx = nvy = nvz = ffloori(radius);
		if(_VG.NVoxelsModified == 0)
			fprintf(stderr, "\t%d x %d x %d voxel cube using grid coordinates\n", 1+2*nvx, 1+2*nvy, 1+2*nvz);
	}
	else {
		// User coordinates may be anisotropic 
		nvx = ffloori(radius / _VG.P_CurrentVGrid->vxdelta);
		nvy = ffloori(radius / _VG.P_CurrentVGrid->vydelta);
		nvz = ffloori(radius / _VG.P_CurrentVGrid->vzdelta);
		// Only print once 
		if(_VG.NVoxelsModified == 0) {
			double anisotropy = (double)(MIN(nvx, MIN(nvy, nvz))) / (double)(MAX(nvx, MAX(nvy, nvz)));
			fprintf(stderr, "\tradius %g gives a brick of %d voxels on x, %d voxels on y, %d voxels on z\n", radius, 1+2*nvx, 1+2*nvy, 1+2*nvz);
			if(anisotropy < 0.4)
				fprintf(stderr, "Warning:\n\tvoxel grid spacing on x, y, and z is very anisotropic.\n\tConsider using vgfill rather than vfill\n");
		}
	}
	// This can be a HUGE iteration, in which case resetting the
	// FPE trap handler on every voxel evaluateion can be a
	// significant performance bottleneck (why??).
	save_fpe_trap = _Df.df_nofpe_trap;
	_Df.df_nofpe_trap = true;
	// The iteration covers a cube rather than a sphere 
	_Df.evaluate_inside_using = true;
	for(ix = ivx - nvx; ix <= ivx + nvx; ix++) {
		if(ix >= 0 && ix < N) {
			for(iy = ivy - nvy; iy <= ivy + nvy; iy++) {
				if(iy >= 0 && iy < N) {
					for(iz = ivz - nvz; iz <= ivz + nvz; iz++) {
						int index;
						if(iz >= 0 && iz < N) {
							index = ix + iy * N + iz * N*N;
							if(index >= 0 && index <= N*N*N) {
								t_voxel * p_voxel = &_VG.P_CurrentVGrid->vdata[index];
								// vx, vy, vz are the true coordinates of this voxel 
								const double vx = _VG.P_CurrentVGrid->vxmin + ix * _VG.P_CurrentVGrid->vxdelta;
								const double vy = _VG.P_CurrentVGrid->vymin + iy * _VG.P_CurrentVGrid->vydelta;
								const double vz = _VG.P_CurrentVGrid->vzmin + iz * _VG.P_CurrentVGrid->vzdelta;
								const double distance = sqrt((vx-x)*(vx-x) + (vy-y)*(vy-y) + (vz-z)*(vz-z));
								double grid_dist = sqrt((gvx-ix)*(gvx-ix) + (gvy-iy)*(gvy-iy) + (gvz-iz)*(gvz-iz));
								// Limit the summation to a sphere rather than a cube
								// but always include the voxel nearest the target    
								if(gridcoordinates) { // Grid coordinates 
									if(grid_dist > radius && (ix != ivx || iy != ivy || iz != ivz))
										continue;
								}
								else { // User coordinates 
									if(distance > radius && (ix != ivx || iy != ivy || iz != ivz))
										continue;
								}
								// Store in user variable VoxelDistance for use by density_function 
								_VG.P_UdvVoxelDistance->udv_value.v.cmplx_val.real = distance;
								_VG.P_UdvGridDistance->udv_value.v.cmplx_val.real = grid_dist;
								EvaluateAt(_VG.P_DensityFunction, &a);
								*p_voxel = static_cast<t_voxel>(*p_voxel + Real(&a));
								// Bookkeeping 
								_VG.NVoxelsModified++;
							}
						}
					}
				}
			}
		}
	}
	_Df.df_nofpe_trap = save_fpe_trap;
	_Df.evaluate_inside_using = false;
}

#endif /* VOXEL_GRID_SUPPORT */

#ifndef VOXEL_GRID_SUPPORT
#define NO_SUPPORT IntError(NO_CARET, "this gnuplot does not support voxel grids")
void GnuPlot::CheckGridRanges() { NO_SUPPORT; }
void GnuPlot::SetVGrid() { NO_SUPPORT; }
void GnuPlot::SetVGridRange() { NO_SUPPORT; }
void GnuPlot::ShowVGrid() { NO_SUPPORT; }
void GnuPlot::ShowIsoSurface() { NO_SUPPORT; }
void GnuPlot::VoxelCommand() { NO_SUPPORT; }
void GnuPlot::VFillCommand() { NO_SUPPORT; }
void GnuPlot::VClearCommand() {}
void GnuPlot::UnsetVGrid() {}
void GnuPlot::InitVoxelSupport() {}
void GnuPlot::SetIsoSurface() {}
//udvt_entry * get_vgrid_by_name(char * c) { return NULL; }
//void gpfree_vgrid(struct udvt_entry * x) {
}

#endif /* no VOXEL_GRID_SUPPORT */
