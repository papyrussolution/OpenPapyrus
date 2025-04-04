// FIT.C
// 
/*
 *    Nonlinear least squares fit according to the Marquardt-Levenberg-algorithm
 *
 *      added as Patch to Gnuplot (v3.2 and higher) by Carsten Grammes
 *
 * Michele Marziani (marziani@ferrara.infn.it), 930726: Recoding of the
 * Unix-like raw console I/O routines.
 *
 * drd: start unitialised variables at 1 rather than NEARLY_ZERO
 *  (fit is more likely to converge if started from 1 than 1e-30 ?)
 *
 * HBB (broeker@physik.rwth-aachen.de) : fit didn't calculate the errors
 * in the 'physically correct' (:-) way, if a third data column containing
 * the errors (or 'uncertainties') of the input data was given. I think
 * I've fixed that, but I'm not sure I really understood the M-L-algo well
 * enough to get it right. I deduced my change from the final steps of the
 * equivalent algorithm for the linear case, which is much easier to
 * understand. (I also made some minor, mostly cosmetic changes)
 *
 * HBB (again): added error checking for negative covar[i][i] values and
 * for too many parameters being specified.
 *
 * drd: allow 3d fitting. Data value is now called fit_z internally,
 * ie a 2d fit is z vs x, and a 3d fit is z vs x and y.
 *
 * HBB, 971023: lifted fixed limit on number of datapoints, and number
 * of parameters.
 *
 * HBB/H.Harders, 20020927: log file name now changeable from inside
 * gnuplot, not only by setting an environment variable.
 *
 * Jim Van Zandt, 090201: allow fitting functions with up to five
 * independent variables.
 *
 * Carl Michal, 120311: optionally prescale all the parameters that
 * the L-M routine sees by their initial values, so that parameters
 * that differ by many orders of magnitude do not cause problems.
 * With decent initial guesses, fits often take fewer iterations. If
 * any variables were 0, then don't do it for those variables, since
 * it may do more harm than good.
 *
 * Thomas Mattison, 130421: brief one-line reports, based on patchset #230.
 * Bastian Maerkisch, 130421: different output verbosity levels
 *
 * Bastian Maerkisch, 130427: remember parameters etc. of last fit and use
 * this data in a subsequent update command if the parameter file does not
 * exist yet.
 *
 * Thomas Mattison, 130508: New convergence criterion which is absolute
 * reduction in chisquare for an iteration of less than epsilon*chisquare
 * plus epsilon_abs (new setting).  The default convergence criterion is
 * always relative no matter what the chisquare is, but users now have the
 * flexibility of adding an absolute convergence criterion through
 * `set fit limit_abs`. Patchset #230.
 *
 * Ethan A Merritt, June 2013: Remove limit of 5 independent parameters.
 * The limit is now the smaller of MAXDATACOLS-2 and MAX_NUM_VAR.
 * Dissociate parameters other than x/y from "normal" plot axes.
 * To refine more than 2 parameters, name them with `set dummy`.
 * x and y still refer to AxS[] in order to allow time/date formats.
 *
 * Bastian Maerkisch, Feb 2014: New syntax to specify errors. The new
 * parameter 'errors' accepts a comma separated list of (dummy) variables
 * to specify which (in-)dependent variable has associated errors. 'z'
 * always denotes the indep. variable. 'unitweights' tells fit to use equal
 * (1) weights for the fit. The new syntax removes the ambiguity between
 * x:y:z:(1) and x:z:s. The old syntax is still accepted but deprecated.
 *
 * Alexander Taeschner, Feb 2014: Optionally take errors of independent
 * variables into account.
 *
 * Bastian Maerkisch, Feb 2014: Split regress() into several functions
 * in order to facilitate the inclusion of alternative fitting codes.
 *
 * Karl Ratzsch, May 2014: Add a result variable reporting the number of
 * iterations
 *
 */
#include <gnuplot.h>
#pragma hdrstop

// Just temporary 
//static void Dblfn(const char * fmt, ...);
//#define Dblf  Dblfn
//#define Dblf2 Dblfn
//#define Dblf3 Dblfn
//#define Dblf5 Dblfn
//#define Dblf6 Dblfn
#ifdef _WIN32
	#include <fcntl.h>
	#include "win/winmain.h"
#endif

#ifdef INFINITY
	#undef INFINITY
#endif
#define INFINITY    1e30
#define NEARLY_ZERO 1e-30
#define INITIAL_VALUE 1.0 // create new variables with this value (was NEARLY_ZERO) 
#define DELTA       0.001 // Relative change for derivatives 
#define MAX_DATA    2048
#define MAX_PARAMS  32
#define MAX_LAMBDA  1e20
#define MIN_LAMBDA  1e-20
#define PLUSMINUS   "+/-"
#define LASTFITCMDLENGTH 511
#define STANDARD stderr // compatible with gnuplot philosophy 
#define BACKUP_SUFFIX ".old" // Suffix of a backup file 
#define SQR(x) ((x) * (x))
//
// fit control 
//
//char * fitlogfile = NULL;
//verbosity_level fit_verbosity = BRIEF;
//char * fit_script = NULL;
//int    fit_wrap = 0;
//int    maxiter = 0;
//double epsilon_abs = 0.0; // default to zero non-relative limit 
//bool   fit_suppress_log = FALSE;
//bool   fit_errorvariables = TRUE;
//bool   fit_covarvariables = FALSE;
//bool   fit_errorscaling = TRUE;
//bool   fit_prescale = TRUE;
//bool   fit_v4compatible = FALSE;
//
// names of user control variables 
//
const char * FITLIMIT = "FIT_LIMIT";
const char * FITSTARTLAMBDA = "FIT_START_LAMBDA";
const char * FITLAMBDAFACTOR = "FIT_LAMBDA_FACTOR";
const char * FITMAXITER = "FIT_MAXITER";
static const char fitlogfile_default[] = "fit.log";
static const char GNUFITLOG[] = "FIT_LOG";
static const char * GP_FIXED = "# FIXED";
static const char * FITSCRIPT = "FIT_SCRIPT";
static const char * DEFAULT_CMD = "replot"; // if no fitscript spec. 

#if 0 // {
static double epsilon = DEF_FIT_LIMIT;  // relative convergence limit 
static double startup_lambda = 0;
static double lambda_down_factor = LAMBDA_DOWN_FACTOR;
static double lambda_up_factor = LAMBDA_UP_FACTOR;
static FILE * log_f = NULL;
static FILE * via_f = NULL;
static bool fit_show_lambda = TRUE;
static int num_data;
static int num_params;
static int num_indep; // # independent variables in fit function 
static int num_errors; // #error columns 
static bool err_cols[MAX_NUM_VAR+1]; // TRUE if variable has an associated error 
static int columns; // # values read from data file for each point 
static double * fit_x = 0; // all independent variable values, e.g. value of the ith variable from the jth data point is in fit_x[j*num_indep+i] 
static double * fit_z = 0; // dependent data values 
static double * err_data = 0; // standard deviations of indep. and dependent data
static double * a = 0; // array of fitting parameters 
static double ** regress_C = 0; // global copy of C matrix in regress 
static void (* regress_cleanup)() = NULL; // memory cleanup function callback 
static bool user_stop = FALSE;
static double * scale_params = 0; // scaling values for parameters 
static udft_entry func;
static fixstr * par_name;
static GpValue ** par_udv; // array of pointers to the "via" variables 
static fixstr * last_par_name = NULL;
static int last_num_params = 0;
static char * last_dummy_var[MAX_NUM_VAR];
static char * last_fit_command = NULL;
// Mar 2014 - the single hottest call path in fit was looking up the
// dummy parameters by name (4 billion times in fit.dem).
// A total waste, since they don't change.  Look up once and store here.
static udvt_entry * fit_dummy_udvs[MAX_NUM_VAR];
#endif // }
//
// internal Prototypes
//
#if !defined(_WIN32) || defined(WGP_CONSOLE)
	static RETSIGTYPE ctrlc_handle(int an_int);
#endif
static void ctrlc_setup();
//
// This is called when a SIGINT occurs during fit
//
#if !defined(_WIN32) || defined(WGP_CONSOLE)
static RETSIGTYPE ctrlc_handle(int an_int)
{
	// reinstall signal handler (necessary on SysV) 
	signal(SIGINT, (sigfunc)ctrlc_handle);
	ctrlc_flag = TRUE;
}
#endif
//
// setup the ctrl_c signal handler
//
static void ctrlc_setup()
{
/*
 *  MSDOS defines signal(SIGINT) but doesn't handle it through
 *  real interrupts. So there remain cases in which a ctrl-c may
 *  be uncaught by signal. We must use kbhit() instead that really
 *  serves the keyboard interrupt (or write an own interrupt func
 *  which also generates #ifdefs)
 *
 *  I hope that other OSes do it better, if not... add #ifdefs :-(
 */
#if (defined(__EMX__) || !defined(MSDOS)) && (!defined(_WIN32) || defined(WGP_CONSOLE))
	signal(SIGINT, (sigfunc)ctrlc_handle);
#endif
}
//
// in case of fatal errors
//
//void error_ex(int t_num, const char * str, ...)
void GnuPlot::ErrorEx(int t_num, const char * str, ...)
{
	char buf[128];
	va_list args;
	va_start(args, str);
	vsnprintf(buf, sizeof(buf), str, args);
	va_end(args);
	// cleanup - free memory 
	if(_Fit.log_f) {
		fprintf(_Fit.log_f, "BREAK: %s", buf);
		SFile::ZClose(&_Fit.log_f);
	}
	SFile::ZClose(&_Fit.via_f);
	ZFREE(_Fit.fit_x);
	ZFREE(_Fit.fit_z);
	ZFREE(_Fit.err_data);
	ZFREE(_Fit.a);
	if(_Fit.func.at) {
		free_at(_Fit.func.at); // release perm. action table 
		_Fit.func.at = (at_type *)NULL;
	}
	if(_Fit.CleanUpProc)
		(this->*_Fit.CleanUpProc)();
	// the datafile may still be open 
	DfClose(_Df);
	// restore original SIGINT function 
	interrupt_setup();
	// FIXME: It would be nice to exit the "fit" command non-fatally, 
	// so that the script who called it can recover and continue.     
	// IntError() makes that impossible.  But if we use IntWarn()   
	// instead the program tries to continue _inside_ the fit, which  
	// generally then dies on some more serious error.                
	// exit via IntError() so that it can clean up state variables 
	IntError(t_num, buf);
}
//
// Marquardt's nonlinear least squares fit
//
//static marq_res_t marquardt(double a[], double ** C, double * chisq, double * lambda)
marq_res_t GnuPlot::Marquardt(GpFit & rFit, double a[], double ** C, double * chisq, double * lambda)
{
	int i, j;
	static double * da = 0; // delta-step of the parameter 
	static double * temp_a = 0; // temptative new params set   
	static double * d = 0;
	static double * tmp_d = 0;
	static double ** tmp_C = 0;
	static double * residues = 0;
	static double ** deriv = 0;
	double tmp_chisq;
	// Initialization when lambda == -1 
	if(*lambda == -1) { /* Get first chi-square check */
		temp_a = vec(rFit.num_params);
		d = vec(rFit.num_data + rFit.num_params);
		tmp_d = vec(rFit.num_data + rFit.num_params);
		da = vec(rFit.num_params);
		residues = vec(rFit.num_data + rFit.num_params);
		tmp_C = matr(rFit.num_data + rFit.num_params, rFit.num_params);
		deriv = NULL;
		if(rFit.num_errors > 1)
			deriv = matr(rFit.num_errors - 1, rFit.num_data);
		Analyze(rFit, a, C, d, chisq, deriv);
		// Calculate a useful startup value for lambda, as given by Schwarz 
		if(rFit.startup_lambda != 0.0)
			*lambda = rFit.startup_lambda;
		else {
			*lambda = 0;
			for(i = 0; i < rFit.num_data; i++)
				for(j = 0; j < rFit.num_params; j++)
					*lambda += C[i][j] * C[i][j];
			*lambda = sqrt(*lambda / rFit.num_data / rFit.num_params);
		}
		// Fill in the lower square part of C (the diagonal is filled in on
		// each iteration, see below) 
		for(i = 0; i < rFit.num_params; i++)
			for(j = 0; j < i; j++)
				C[rFit.num_data + i][j] = 0, C[rFit.num_data + j][i] = 0;
		return OK;
	}
	// once converged, free allocated vars 
	if(*lambda == -2) {
		ZFREE(d);
		ZFREE(tmp_d);
		ZFREE(da);
		ZFREE(temp_a);
		ZFREE(residues);
		free_matr(tmp_C);
		free_matr(deriv);
		// may be called more than once 
		tmp_C = deriv = (double**)NULL;
		return OK;
	}
	// Givens calculates in-place, so make working copies of C and d 
	for(j = 0; j < rFit.num_data + rFit.num_params; j++)
		memcpy(tmp_C[j], C[j], rFit.num_params * sizeof(double));
	memcpy(tmp_d, d, rFit.num_data * sizeof(double));
	// fill in additional parts of tmp_C, tmp_d 
	for(i = 0; i < rFit.num_params; i++) {
		// fill in low diag. of tmp_C ... 
		tmp_C[rFit.num_data + i][i] = *lambda;
		// ... and low part of tmp_d 
		tmp_d[rFit.num_data + i] = 0;
	}
	Givens(tmp_C, tmp_d, da, rFit.num_params + rFit.num_data, rFit.num_params);
	// check if trial did ameliorate sum of squares 
	for(j = 0; j < rFit.num_params; j++)
		temp_a[j] = a[j] + da[j];
	Analyze(rFit, temp_a, tmp_C, tmp_d, &tmp_chisq, deriv);
	// tsm patchset 230: Changed < to <= in next line 
	// so finding exact minimum stops iteration instead of just increasing lambda. 
	// Disadvantage is that if lambda is large enough so that chisq doesn't change 
	// is taken as success. 
	if(tmp_chisq <= *chisq) { /* Success, accept new solution */
		if(*lambda > MIN_LAMBDA) {
			if(rFit.fit_verbosity == VERBOSE)
				putc('/', stderr);
			*lambda /= rFit.lambda_down_factor;
		}
		// update chisq, C, d, a 
		*chisq = tmp_chisq;
		for(j = 0; j < rFit.num_data; j++) {
			memcpy(C[j], tmp_C[j], rFit.num_params * sizeof(double));
			d[j] = tmp_d[j];
		}
		for(j = 0; j < rFit.num_params; j++)
			a[j] = temp_a[j];
		return BETTER;
	}
	else { // failure, increase lambda and return 
		*lambda *= rFit.lambda_up_factor;
		if(rFit.fit_verbosity == VERBOSE)
			putc('*', stderr);
		else if(rFit.fit_verbosity == BRIEF) /* one-line report even if chisq increases */
			FitShowBrief(-1, tmp_chisq, *chisq, temp_a, *lambda, STANDARD);
		return WORSE;
	}
}
//
// compute the (effective) error
//
//static double effective_error(double ** ppDeriv, int i)
double GnuPlot::EffectiveError(GpFit & rFit, double ** ppDeriv, int i)
{
	double tot_err;
	if(rFit.num_errors <= 1) /* z-errors or equal weights */
		tot_err = rFit.err_data[i];
	else {
		// "Effective variance" according to Jay Orear, Am. J. Phys., Vol. 50, No. 10, October 1982
		tot_err = SQR(rFit.err_data[i * rFit.num_errors + (rFit.num_errors - 1)]);
		for(int j = 0, k = 0; j < rFit.num_indep; j++) {
			if(rFit.err_cols[j]) {
				tot_err += SQR(ppDeriv[k][i] * rFit.err_data[i * rFit.num_errors + k]);
				k++;
			}
		}
		tot_err = sqrt(tot_err);
	}
	return tot_err;
}
//
// compute chi-square and numeric derivations
//
// Used by marquardt to evaluate the linearized fitting matrix C and
// vector d. Fills in only the top part of C and d. I don't use a
// temporary array zfunc[] any more. Just use d[] instead.  
//
//static void analyze(double pA[], double ** ppC, double pD[], double * pChisq, double ** ppDeriv)
void GnuPlot::Analyze(GpFit & rFit, double pA[], double ** ppC, double pD[], double * pChisq, double ** ppDeriv)
{
	Calculate(rFit, pD, ppC, pA);
	// derivatives in indep. variables are required for effective variance method 
	if(rFit.num_errors > 1)
		CalcDerivatives(rFit, pA, pD, ppDeriv);
	for(int i = 0; i < rFit.num_data; i++) {
		double err = EffectiveError(rFit, ppDeriv, i);
		// note: order reversed, as used by Schwarz 
		pD[i] = (pD[i] - rFit.fit_z[i]) / err;
		for(int j = 0; j < rFit.num_params; j++)
			ppC[i][j] /= err;
	}
	*pChisq = sumsq_vec(rFit.num_data, pD);
}
//
// compute function values and partial derivatives of chi-square
//
// To use the more exact, but slower two-side formula, activate the
// following line: 
//#define TWO_SIDE_DIFFERENTIATION 
//static void calculate(double * pZFunc, double ** ppDzda, double pA[])
void GnuPlot::Calculate(GpFit & rFit, double * pZFunc, double ** ppDzda, double pA[])
{
	int k, p;
	double tmp_a;
	double * tmp_high = vec(rFit.num_data); /* numeric derivations */
#ifdef TWO_SIDE_DIFFERENTIATION
	double * tmp_low = vec(rFit.num_data);
#endif
	double * tmp_pars = vec(rFit.num_params);
	// first function values 
	Call(rFit, pA, pZFunc);
	// then derivatives in parameters 
	for(p = 0; p < rFit.num_params; p++)
		tmp_pars[p] = pA[p];
	for(p = 0; p < rFit.num_params; p++) {
		tmp_a = fabs(pA[p]) < NEARLY_ZERO ? NEARLY_ZERO : pA[p];
		tmp_pars[p] = tmp_a * (1 + DELTA);
		Call(rFit, tmp_pars, tmp_high);
#ifdef TWO_SIDE_DIFFERENTIATION
		tmp_pars[p] = tmp_a * (1 - DELTA);
		Call(tmp_pars, tmp_low);
#endif
		for(k = 0; k < rFit.num_data; k++)
#ifdef TWO_SIDE_DIFFERENTIATION
			dzda[k][p] = (tmp_high[k] - tmp_low[k]) / (2 * tmp_a * DELTA);
#else
			ppDzda[k][p] = (tmp_high[k] - pZFunc[k]) / (tmp_a * DELTA);
#endif
		tmp_pars[p] = pA[p];
	}
#ifdef TWO_SIDE_DIFFERENTIATION
	SAlloc::F(tmp_low);
#endif
	SAlloc::F(tmp_high);
	SAlloc::F(tmp_pars);
}
//
// call internal gnuplot functions
//
//void call_gnuplot(const double * par, double * data)
void GnuPlot::Call(GpFit & rFit, const double * par, double * data)
{
	int i, j;
	GpValue v;
	// set parameters first 
	for(i = 0; i < rFit.num_params; i++)
		Gcomplex(rFit.par_udv[i], par[i] * rFit.scale_params[i], 0.0);
	for(i = 0; i < rFit.num_data; i++) {
		// calculate fit-function value 
		// initialize extra dummy variables from the corresponding actual variables, if any. 
		for(j = 0; j < MAX_NUM_VAR; j++) {
			double dummy_value;
			udvt_entry * udv = rFit.fit_dummy_udvs[j];
			if(!udv)
				IntError(NO_CARET, "Internal error: lost a dummy parameter!");
			if(udv->udv_value.Type == CMPLX || udv->udv_value.Type == INTGR)
				dummy_value = Real(&udv->udv_value);
			else
				dummy_value = 0.0;
			Gcomplex(&rFit.func.dummy_values[j], dummy_value, 0.0);
		}
		// set actual dummy variables from file data 
		for(j = 0; j < rFit.num_indep; j++)
			Gcomplex(&rFit.func.dummy_values[j], rFit.fit_x[i * rFit.num_indep + j], 0.0);
		EvaluateAt(rFit.func.at, &v);
		if(Ev.IsUndefined_ || isnan(Real(&v))) {
			// Print useful info on undefined-function error. 
			rFit.Dblfn("\nCurrent data point\n");
			rFit.Dblfn("=========================\n");
			rFit.Dblfn("%-15s = %i out of %i\n", "#", i + 1, rFit.num_data);
			for(j = 0; j < rFit.num_indep; j++)
				rFit.Dblfn("%-15.15s = %-15g\n", _Pb.c_dummy_var[j], par[j] * rFit.scale_params[j]);
			rFit.Dblfn("%-15.15s = %-15g\n", "z", rFit.fit_z[i]);
			rFit.Dblfn("\nCurrent set of parameters\n");
			rFit.Dblfn("=========================\n");
			for(j = 0; j < rFit.num_params; j++)
				rFit.Dblfn("%-15.15s = %-15g\n", rFit.par_name[j], par[j] * rFit.scale_params[j]);
			rFit.Dblfn("\n");
			if(Ev.IsUndefined_) {
				Eex("Undefined value during function evaluation");
			}
			else {
				Eex("Function evaluation yields NaN (\"not a number\")");
			}
		}
		data[i] = Real(&v);
	}
}
//
// calculate derivatives wrt the parameters
//
// Used to calculate the effective variance in EffectiveError() 
//
//static void calc_derivatives(const double * pPar, double * pData, double ** ppDeriv)
void GnuPlot::CalcDerivatives(GpFit & rFit, const double * pPar, double * pData, double ** ppDeriv)
{
	int i, j, k, m;
	GpValue v;
	double h;
	// set parameters first 
	for(i = 0; i < rFit.num_params; i++)
		Gcomplex(rFit.par_udv[i], pPar[i] * rFit.scale_params[i], 0.0);
	for(i = 0; i < rFit.num_data; i++) { /* loop over data points */
		for(j = 0, m = 0; j < rFit.num_indep; j++) { /* loop over indep. variables */
			double tmp_high;
			double tmp_x;
#ifdef TWO_SIDE_DIFFERENTIATION
			double tmp_low;
#endif
			// only calculate derivatives if necessary 
			if(!rFit.err_cols[j])
				continue;
			// set actual dummy variables from file data 
			for(k = 0; k < rFit.num_indep; k++) {
				if(j != k)
					Gcomplex(&rFit.func.dummy_values[k], rFit.fit_x[i * rFit.num_indep + k], 0.0);
			}
			tmp_x = rFit.fit_x[i * rFit.num_indep + j];
			// optimal step size 
			h = MAX(DELTA * fabs(tmp_x), 8*1e-8*(fabs(tmp_x) + 1e-8));
			Gcomplex(&rFit.func.dummy_values[j], tmp_x + h, 0.0);
			EvaluateAt(rFit.func.at, &v);
			tmp_high = Real(&v);
#ifdef TWO_SIDE_DIFFERENTIATION
			Gcomplex(&func.dummy_values[j], tmp_x - h, 0.0);
			EvaluateAt(func.at, &v);
			tmp_low = Real(&v);
			ppDeriv[m][i] = (tmp_high - tmp_low) / (2 * h);
#else
			ppDeriv[m][i] = (tmp_high - pData[i]) / h;
#endif
			m++;
		}
	}
}
//
// handle user interrupts during fit
//
//static bool fit_interrupt()
bool GnuPlot::FitInterrupt(GpFit & rFit)
{
	while(TRUE) {
		fputs("\n\n(S)top fit, (C)ontinue, (E)xecute FIT_SCRIPT:  ", STANDARD);
#ifdef _WIN32
		WinRaiseConsole();
#endif
		switch(getchar()) {
			case EOF:
			case 's':
			case 'S':
			    fputs("Stop.\n", STANDARD);
			    rFit.user_stop = TRUE;
			    return FALSE;
			case 'c':
			case 'C':
			    fputs("Continue.\n", STANDARD);
			    return TRUE;
			case 'e':
			case 'E': 
				{
					const char * tmp = GetFitScript(rFit);
					fprintf(STANDARD, "executing: %s\n", tmp);
					// FIXME: Shouldn't we also set FIT_STDFIT etc? 
					// set parameters visible to gnuplot 
					for(int i = 0; i < rFit.num_params; i++)
						Gcomplex(rFit.par_udv[i], rFit.a[i] * rFit.scale_params[i], 0.0);
					DoString(tmp);
				}
		}
	}
	return TRUE;
}
//
// determine current setting of FIT_SCRIPT
//
//const char * getfitscript()
const char * GnuPlot::GetFitScript(GpFit & rFit)
{
	if(rFit.fit_script)
		return rFit.fit_script;
	else {
		char * tmp = getenv(FITSCRIPT);
		return NZOR(tmp, DEFAULT_CMD);
	}
}
//
// initial setup for regress()
//
//static void regress_init()
void GnuPlot::RegressInit(GpFit & rFit)
{
	// Reset flag describing fit result status 
	udvt_entry * v = Ev.AddUdvByName("FIT_CONVERGED"); // For exporting results to the user 
	Ginteger(&v->udv_value, 0);
	// Ctrl-C now serves as Hotkey 
	ctrlc_setup();
	// HBB 981118: initialize new variable 'user_break' 
	rFit.user_stop = false;
}
//
// finalize regression: print results and set user variables
//
//static void regress_finalize(int iter, double chisq, double last_chisq, double lambda, double ** covar)
void GnuPlot::RegressFinalize(GpFit & rFit, int iter, double chisq, double lastChisq, double lambda, double ** ppCovar)
{
	int i, j;
	udvt_entry * v; // For exporting results to the user 
	int ndf;
	int niter;
	double stdfit;
	double pvalue;
	double * dpar;
	double ** corel = NULL;
	bool covar_invalid = false;
	interrupt_setup(); // restore original SIGINT function 
	// tsm patchset 230: final progress report labels to console 
	if(rFit.fit_verbosity == BRIEF)
		FitShowBrief(-2, chisq, chisq, rFit.a, lambda, STANDARD);
	// tsm patchset 230: final progress report to log file 
	if(!rFit.fit_suppress_log) {
		if(rFit.fit_verbosity == VERBOSE)
			FitShow(iter, chisq, lastChisq, rFit.a, lambda, rFit.log_f);
		else
			FitShowBrief(iter, chisq, lastChisq, rFit.a, lambda, rFit.log_f);
	}
	// test covariance matrix 
	if(ppCovar) {
		for(i = 0; i < rFit.num_params; i++) {
			// diagonal elements must be larger than zero 
			if(ppCovar[i][i] <= 0.0) {
				// Not a fatal error, but prevent floating point exception later on 
				rFit.Dblfn("Calculation error: non-positive diagonal element in covar. matrix of parameter '%s'.\n", rFit.par_name[i]);
				covar_invalid = TRUE;
			}
		}
	}
	// HBB 970304: the maxiter patch: 
	if(rFit.maxiter > 0 && iter > rFit.maxiter) {
		rFit.Dblfn("\nMaximum iteration count (%d) reached. Fit stopped.\n", rFit.maxiter);
	}
	else if(rFit.user_stop) {
		rFit.Dblfn("\nThe fit was stopped by the user after %d iterations.\n", iter);
	}
	else if(lambda >= MAX_LAMBDA) {
		rFit.Dblfn("\nThe maximum lambda = %e was exceeded. Fit stopped.\n", MAX_LAMBDA);
	}
	else if(covar_invalid) {
		rFit.Dblfn("\nThe covariance matrix is invalid. Fit did not converge properly.\n");
	}
	else {
		rFit.Dblfn("\nAfter %d iterations the fit converged.\n", iter);
		v = Ev.AddUdvByName("FIT_CONVERGED");
		Ginteger(&v->udv_value, 1);
	}
	// fit results 
	ndf    = rFit.num_data - rFit.num_params;
	stdfit = sqrt(chisq / ndf);
	pvalue = 1.0 - chisq_cdf(ndf, chisq);
	niter = iter;
	// Export these to user-accessible variables 
	v = Ev.AddUdvByName("FIT_NDF");
	Ginteger(&v->udv_value, ndf);
	v = Ev.AddUdvByName("FIT_STDFIT");
	Gcomplex(&v->udv_value, stdfit, 0);
	v = Ev.AddUdvByName("FIT_WSSR");
	Gcomplex(&v->udv_value, chisq, 0);
	v = Ev.AddUdvByName("FIT_P");
	Gcomplex(&v->udv_value, pvalue, 0);
	v = Ev.AddUdvByName("FIT_NITER");
	Ginteger(&v->udv_value, niter);
	// Save final parameters. Depending on the backend and
	// its internal state, the last call_gnuplot may not have been
	// at the minimum 
	for(i = 0; i < rFit.num_params; i++)
		Gcomplex(rFit.par_udv[i], rFit.a[i] * rFit.scale_params[i], 0.0);
	// Set error and covariance variables to zero,
	// thus making sure they are created. 
	if(rFit.fit_errorvariables) {
		for(i = 0; i < rFit.num_params; i++)
			SetVarErr(rFit.par_name[i], 0.0);
	}
	if(rFit.fit_covarvariables) {
		// first, remove all previous covariance variables 
		DelUdvByName("FIT_COV_*", TRUE);
		for(i = 0; i < rFit.num_params; i++) {
			for(j = 0; j < i; j++) {
				SetVarCovar(rFit.par_name[i], rFit.par_name[j], 0.0);
				SetVarCovar(rFit.par_name[j], rFit.par_name[i], 0.0);
			}
			SetVarCovar(rFit.par_name[i], rFit.par_name[i], 0.0);
		}
	}
	// calculate unscaled parameter errors in dpar[]: 
	dpar = vec(rFit.num_params);
	if(ppCovar && !covar_invalid) {
		// calculate unscaled parameter errors in dpar[]: 
		for(i = 0; i < rFit.num_params; i++) {
			dpar[i] = sqrt(ppCovar[i][i]);
		}
		// transform covariances into correlations 
		corel = matr(rFit.num_params, rFit.num_params);
		for(i = 0; i < rFit.num_params; i++) {
			// only lower triangle needs to be handled 
			for(j = 0; j < i; j++)
				corel[i][j] = ppCovar[i][j] / (dpar[i] * dpar[j]);
			corel[i][i] = 1.0;
		}
	}
	else {
		// set all errors to zero if covariance matrix invalid or unavailable 
		for(i = 0; i < rFit.num_params; i++)
			dpar[i] = 0.0;
	}
	if(rFit.fit_errorscaling || (rFit.num_errors == 0)) {
		// scale parameter errors based on chisq 
		const double temp = sqrt(chisq / (rFit.num_data - rFit.num_params));
		for(i = 0; i < rFit.num_params; i++)
			dpar[i] *= temp;
	}
	// Save user error variables. 
	if(rFit.fit_errorvariables) {
		for(i = 0; i < rFit.num_params; i++)
			SetVarErr(rFit.par_name[i], dpar[i] * rFit.scale_params[i]);
	}
	// fill covariance variables if needed 
	if(rFit.fit_covarvariables && ppCovar && !covar_invalid) {
		const double scale = (rFit.fit_errorscaling || (rFit.num_errors == 0)) ? (chisq / (rFit.num_data - rFit.num_params)) : 1.0;
		for(i = 0; i < rFit.num_params; i++) {
			// only lower triangle needs to be handled 
			for(j = 0; j <= i; j++) {
				const double temp = scale * rFit.scale_params[i] * rFit.scale_params[j];
				SetVarCovar(rFit.par_name[i], rFit.par_name[j], ppCovar[i][j] * temp);
				SetVarCovar(rFit.par_name[j], rFit.par_name[i], ppCovar[i][j] * temp);
			}
		}
	}
	rFit.ShowResults(chisq, lastChisq, rFit.a, dpar, corel);
	SAlloc::F(dpar);
	free_matr(corel);
}
//
// test for user request to stop the fit
//
//bool regress_check_stop(int iter, double chisq, double last_chisq, double lambda)
bool GnuPlot::RegressCheckStop(int iter, double chisq, double last_chisq, double lambda)
{
#ifdef _WIN32
	// This call makes the Windows GUI functional during fits. Pressing Ctrl-Break now finally has an effect. 
	WinMessageLoop();
#endif
	if(_Plt.ctrlc_flag) {
		// Always report on current status. 
		if(_Fit.fit_verbosity == VERBOSE)
			FitShow(iter, chisq, last_chisq, _Fit.a, lambda, STANDARD);
		else
			FitShowBrief(iter, chisq, last_chisq, _Fit.a, lambda, STANDARD);
		_Plt.ctrlc_flag = false;
		if(!FitInterrupt(_Fit)) // handle keys 
			return FALSE;
	}
	return TRUE;
}
//
// free memory allocated by gnuplot's internal fitting code
//
//static void internal_cleanup()
void GnuPlot::InternalRegressCleanUp()
{
	free_matr(_Fit.regress_C);
	_Fit.regress_C = NULL;
	double lambda = -2.0; // flag value, meaning 'destruct!' 
	Marquardt(_Fit, NULL, NULL, NULL, &lambda);
}
//
// frame routine for the marquardt-fit
//
//static bool regress(double a[])
bool GnuPlot::Regress(GpFit & rFit, double a[])
{
	double ** covar;
	double ** C;
	double chisq;
	double last_chisq;
	double lambda;
	int iter;
	marq_res_t res;
	rFit.CleanUpProc = &GnuPlot::InternalRegressCleanUp;
	chisq = last_chisq = INFINITY;
	// the global copy to is accessible to error_ex, too 
	rFit.regress_C = C = matr(rFit.num_data + rFit.num_params, rFit.num_params);
	lambda = -1; // use sign as flag 
	iter = 0;    // iteration counter  
	// Initialize internal variables and 1st chi-square check 
	if((res = Marquardt(rFit, a, C, &chisq, &lambda)) == ML_ERROR)
		Eex("FIT: error occurred during fit");
	res = BETTER;
	rFit.fit_show_lambda = TRUE;
	FitProgress(iter, chisq, chisq, a, lambda, STANDARD);
	if(!rFit.fit_suppress_log)
		FitProgress(iter, chisq, chisq, a, lambda, rFit.log_f);
	RegressInit(rFit);
	// MAIN FIT LOOP: do the regression iteration 
	do {
		if(!RegressCheckStop(iter, chisq, last_chisq, lambda))
			break;
		if(res == BETTER) {
			iter++;
			last_chisq = chisq;
		}
		if((res = Marquardt(rFit, a, C, &chisq, &lambda)) == BETTER)
			FitProgress(iter, chisq, last_chisq, a, lambda, STANDARD);
	} while((res != ML_ERROR) && (lambda < MAX_LAMBDA) && ((rFit.maxiter == 0) || (iter <= rFit.maxiter)) && (chisq != 0) && (res == WORSE ||
	    /* tsm patchset 230: change to new convergence criterion */
	    ((last_chisq - chisq) > (rFit.epsilon * chisq + rFit.epsilon_abs))));
	// fit done 
	if(res == ML_ERROR)
		Eex("FIT: error occurred during fit");
	// compute errors in the parameters
	// get covariance-, correlation- and curvature-matrix
	// and errors in the parameters
	// compute covar[][] directly from C 
	Givens(C, 0, 0, rFit.num_data, rFit.num_params);
	// Use lower square of C for covar 
	covar = C + rFit.num_data;
	Invert_RtR(C, covar, rFit.num_params);
	RegressFinalize(rFit, iter, chisq, last_chisq, lambda, covar);
	// call destructor for allocated vars 
	InternalRegressCleanUp();
	rFit.CleanUpProc = 0;
	return TRUE;
}
// 
// display results of the fit
// 
//static void show_results(double chisq, double last_chisq, double * pA, double * pDPar, double ** ppCorel)
void GnuPlot::GpFit::ShowResults(double chisq, double last_chisq, double * pA, double * pDPar, double ** ppCorel)
{
	int i, j, k;
	bool have_errors = TRUE;
	Dblfn("final sum of squares of residuals : %g\n", chisq);
	if(chisq > NEARLY_ZERO) {
		Dblfn("rel. change during last iteration : %g\n\n", (chisq - last_chisq) / chisq);
	}
	else {
		Dblfn("abs. change during last iteration : %g\n\n", (chisq - last_chisq));
	}
	if(num_data == num_params && ((num_errors == 0) || fit_errorscaling)) {
		Dblfn("\nExactly as many data points as there are parameters.\n");
		Dblfn("In this degenerate case, all errors are zero by definition.\n\n");
		have_errors = FALSE;
	}
	else if((chisq < NEARLY_ZERO) && ((num_errors == 0) || fit_errorscaling)) {
		Dblfn("\nHmmmm.... Sum of squared residuals is zero. Can't compute errors.\n\n");
		have_errors = FALSE;
	}
	else if(ppCorel == NULL) {
		Dblfn("\nCovariance matric unavailable. Can't compute errors.\n\n");
		have_errors = FALSE;
	}
	if(!have_errors) {
		Dblfn("Final set of parameters \n");
		Dblfn("======================= \n\n");
		for(k = 0; k < num_params; k++)
			Dblfn("%-15.15s = %-15g\n", par_name[k], pA[k] * scale_params[k]);
	}
	else {
		int ndf  = num_data - num_params;
		double stdfit    = sqrt(chisq/ndf);
		double pvalue    = 1.0 - chisq_cdf(ndf, chisq);
		Dblfn("degrees of freedom    (FIT_NDF)                        : %d\n", ndf);
		Dblfn("rms of residuals      (FIT_STDFIT) = sqrt(WSSR/ndf)    : %g\n", stdfit);
		Dblfn("variance of residuals (reduced chisquare) = WSSR/ndf   : %g\n", chisq / ndf);
		// We cannot know if the errors supplied by the user are weighting factors
		// or real errors, so we print the p-value in any case, although it does not
		// make much sense in the first case.  This means that we print this for x:y:z:(1)
		// fits without errors using the old syntax since this requires 4 columns. 
		if(num_errors > 0)
			Dblfn("p-value of the Chisq distribution (FIT_P)              : %g\n", pvalue);
		Dblfn("\n");
		if(fit_errorscaling || (num_errors == 0))
			Dblfn("Final set of parameters            Asymptotic Standard Error\n");
		else
			Dblfn("Final set of parameters            Standard Deviation\n");
		Dblfn("=======================            ==========================\n");
		for(i = 0; i < num_params; i++) {
			double temp = (fabs(pA[i]) < NEARLY_ZERO) ? 0.0 : fabs(100.0 * pDPar[i] / pA[i]);
			Dblfn("%-15.15s = %-15g  %-3.3s %-12.4g (%.4g%%)\n", par_name[i], pA[i] * scale_params[i], PLUSMINUS, pDPar[i] * scale_params[i], temp);
		}
		// Print correlation matrix only if there is more than one parameter. 
		if((num_params > 1) && ppCorel) {
			Dblfn("\ncorrelation matrix of the fit parameters:\n");
			Dblfn("                ");
			for(j = 0; j < num_params; j++)
				Dblfn("%-6.6s ", par_name[j]);
			Dblfn("\n");
			for(i = 0; i < num_params; i++) {
				Dblfn("%-15.15s", par_name[i]);
				for(j = 0; j <= i; j++) {
					// Only print lower triangle of symmetric matrix 
					Dblfn("%6.3f ", ppCorel[i][j]);
				}
				Dblfn("\n");
			}
		}
	}
}
//
// display actual state of the fit
//
//void fit_progress(int i, double chisq, double last_chisq, double * pA, double lambda, FILE * pDevice)
void GnuPlot::FitProgress(int i, double chisq, double last_chisq, double * pA, double lambda, FILE * pDevice)
{
	if(_Fit.fit_verbosity == VERBOSE)
		FitShow(i, chisq, last_chisq, pA, lambda, pDevice);
	else if(_Fit.fit_verbosity == BRIEF)
		FitShowBrief(i, chisq, last_chisq, pA, lambda, pDevice);
}

//static void fit_show(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device)
void GnuPlot::FitShow(int i, double chisq, double last_chisq, double * pA, double lambda, FILE * pDevice)
{
	fprintf(pDevice,
	    "\n\n\
 Iteration %d\n\
 WSSR        : %-15g   delta(WSSR)/WSSR   : %g\n\
 delta(WSSR) : %-15g   limit for stopping : %g\n",
	    i, chisq, (chisq > NEARLY_ZERO) ? (chisq - last_chisq) / chisq : 0.0, chisq - last_chisq, _Fit.epsilon);
	if(_Fit.fit_show_lambda)
		fprintf(pDevice, "\
 lambda	     : %g\n", lambda);
	fprintf(pDevice, "\n\
%s parameter values\n\n", (i > 0 ? "resultant" : "initial set of free"));
	for(int k = 0; k < _Fit.num_params; k++)
		fprintf(pDevice, "%-15.15s = %g\n", _Fit.par_name[k], pA[k] * _Fit.scale_params[k]);
}
//
// If the exponent of a floating point number in scientific format (%e) has three
// digits and the highest digit is zero, it will get removed by this routine. 
//
static char * pack_float(char * num)
{
	static int needs_packing = -1; // @global
	if(needs_packing < 0) {
		// perform the test only once 
		char buf[12];
		snprintf(buf, sizeof(buf), "%.2e", 1.00); /* "1.00e+000" or "1.00e+00" */
		needs_packing = (strlen(buf) == 9);
	}
	if(needs_packing) {
		char * p = sstrchr(num, 'e');
		SETIFZ(p, sstrchr(num, 'E'));
		if(p) {
			p += 2; // also skip sign of exponent 
			if(*p == '0') {
				do {
					*p = *(p + 1);
				} while(*++p);
			}
		}
	}
	return num;
}
// 
// tsm patchset 230: new one-line version of progress report 
// 
//static void fit_show_brief(int iter, double chisq, double last_chisq, double * pParms, double lambda, FILE * pDevice)
void GnuPlot::FitShowBrief(int iter, double chisq, double last_chisq, double * pParms, double lambda, FILE * pDevice)
{
	int k, len;
	double delta, lim;
	char buf[256];
	char * p;
	const int indent = 4;
	// on iteration 0 or -2, print labels 
	if(iter == 0 || iter == -2) {
		strcpy(buf, "iter      chisq       delta/lim ");
		// 9999 1.1234567890e+00 -1.12e+00 
		if(_Fit.fit_show_lambda)
			strcat(buf, " lambda  ");
		// 1.00e+00 
		fputs(buf, pDevice);
		len = strlen(buf);
		for(k = 0; k < _Fit.num_params; k++) {
			snprintf(buf, sizeof(buf), " %-13.13s", _Fit.par_name[k]);
			len += strlen(buf);
			if(_Fit.fit_wrap > 0 && len >= _Fit.fit_wrap) {
				fprintf(pDevice, "\n%*c", indent, ' ');
				len = indent;
			}
			fputs(buf, pDevice);
		}
		fputs("\n", pDevice);
	}
	if(iter != -2) { // on iteration -2, don't print anything else 
		// new convergence test quantities 
		delta = chisq - last_chisq;
		lim = _Fit.epsilon * chisq + _Fit.epsilon_abs;
		// print values 
		if(iter >= 0)
			snprintf(buf, sizeof(buf), "%4i", iter);
		else // -1 indicates that chisquare increased 
			snprintf(buf, sizeof(buf), "%4c", '*');
		snprintf(buf + 4, sizeof(buf) - 4, " %-17.10e %- 10.2e", chisq, delta / lim);
		if(_Fit.fit_show_lambda)
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %-9.2e", lambda);
		for(k = 0, p = buf + 4; (k < 3) && p; k++) {
			p++;
			pack_float(p);
			p = sstrchr(p, 'e');
		}
		fputs(buf, pDevice);
		len = strlen(buf);
		for(k = 0; k < _Fit.num_params; k++) {
			snprintf(buf, sizeof(buf), " % 14.6e", pParms[k] * _Fit.scale_params[k]);
			pack_float(buf);
			len += strlen(buf);
			if((_Fit.fit_wrap > 0) && (len >= _Fit.fit_wrap)) {
				fprintf(pDevice, "\n%*c", indent, ' ');
				len = indent;
			}
			fputs(buf, pDevice);
		}
		fputs("\n", pDevice);
	}
}
// 
// get next word of a multi-word string, advance pointer
// 
static char * get_next_word(char ** s, char * subst)
{
	char * tmp = *s;
	while(*tmp == ' ' || *tmp == '\t' || *tmp == '=')
		tmp++;
	if(*tmp == '\n' || *tmp == '\r' || *tmp == '\0') /* not found */
		return NULL;
	if((*s = strpbrk(tmp, " =\t\n\r[")) == NULL)
		*s = tmp + strlen(tmp);
	*subst = **s;
	*(*s)++ = '\0';
	return tmp;
}
// 
// first time settings
// 
//void init_fit()
void GnuPlot::InitFit()
{
	_Fit.func.at = (at_type *)NULL; // need to parse 1 time 
}
// 
// Set a GNUPLOT user-defined variable
// 
//static void setvar(char * pVarName, double data)
void GnuPlot::SetVar(const char * pVarName, double data)
{
	// Sanitize name to remove square brackets from array variables 
	SString temp_buf(pVarName);
	temp_buf.ReplaceChar('[', '_').ReplaceChar(']', '_');
	/*for(char * c = pVarName; *c; c++) {
		if(*c == '[' || *c == ']')
			*c = '_';
	}*/
	Ev.FillGpValFoat(/*pVarName*/temp_buf, data);
}
// 
// Set a user-defined variable from an error variable:
// Take the parameter name, turn it  into an error parameter
// name (e.g. a to a_err) and then set it.
// 
//static void setvarerr(const char * varname, double value)
void GnuPlot::SetVarErr(const char * pVarName, double value)
{
	// Create the variable name by appending _err 
	char * pErrValName = (char *)SAlloc::M(strlen(pVarName) + 6);
	sprintf(pErrValName, "%s_err", pVarName);
	SetVar(pErrValName, value);
	SAlloc::F(pErrValName);
}
// 
// Set a user-defined covariance variable:
// Take the two parameter names, turn them into an covariance
// parameter name (e.g. a and b to FIT_COV_a_b) and then set it.
// 
//static void setvarcovar(char * varname1, char * varname2, double value)
void GnuPlot::SetVarCovar(const char * pVarName1, const char * pVarName2, double value)
{
	// The name of the (new) covariance variable 
	char * pCovValName = (char *)SAlloc::M(strlen(pVarName1) + strlen(pVarName2) + 10);
	sprintf(pCovValName, "FIT_COV_%s_%s", pVarName1, pVarName2);
	SetVar(pCovValName, value);
	SAlloc::F(pCovValName);
}
//
// Get integer variable value
//
//static intgr_t getivar(const char * varname)
intgr_t GnuPlot::GetIVar(const char * pVarName)
{
	udvt_entry * v = Ev.GetUdvByName(pVarName);
	return (v && v->udv_value.Type != NOTDEFINED) ? (intgr_t)Real(&v->udv_value) : 0;
}
//
// Get double variable value
//
//static double getdvar(const char * varname)
double GnuPlot::GetDVar(const char * pVarName)
{
	udvt_entry * v = Ev.GetUdvByName(pVarName);
	return (v && v->udv_value.Type != NOTDEFINED) ? Real(&v->udv_value) : 0.0;
}
//
// like GnuPlot::GetDVar, but
//  - create it and set to `value` if not found or undefined
//  - convert it from integer to real if necessary
//
//static double createdvar(const char * pVarName, double value)
double GnuPlot::CreateDVar(const char * pVarName, double value)
{
	udvt_entry * p_udv_ptr = Ev.AddUdvByName(pVarName);
	if(p_udv_ptr->udv_value.Type == NOTDEFINED) // new variable 
		Gcomplex(&p_udv_ptr->udv_value, value, 0.0);
	else if(p_udv_ptr->udv_value.Type == INTGR) // convert to CMPLX 
		Gcomplex(&p_udv_ptr->udv_value, (double)p_udv_ptr->udv_value.v.int_val, 0.0);
	return Real(&p_udv_ptr->udv_value);
}
//
// Modified from save.c:save_range() 
//
//static void log_axis_restriction(FILE * log_f, int param, double min, double max, int autoscale, char * pName)
void GnuPlot::LogAxisRestriction(FILE * log_f, int param, double min, double max, int autoscale, const char * pName)
{
	char s[80];
	// FIXME: Is it really worth it to format time values? 
	GpAxis * axis = (param == 1) ? &AxS.__Y() : &AxS.__X();
	fprintf(log_f, "        %s range restricted to [", pName);
	if(autoscale & AUTOSCALE_MIN) {
		putc('*', log_f);
	}
	else if(param < 2 && axis->datatype == DT_TIMEDATE) {
		putc('"', log_f);
		GStrFTime(s, 80, AxS.P_TimeFormat, min);
		fputs(s, log_f);
		putc('"', log_f);
	}
	else {
		fprintf(log_f, "%#g", min);
	}
	fputs(" : ", log_f);
	if(autoscale & AUTOSCALE_MAX) {
		putc('*', log_f);
	}
	else if(param < 2 && axis->datatype == DT_TIMEDATE) {
		putc('"', log_f);
		GStrFTime(s, 80, AxS.P_TimeFormat, max);
		fputs(s, log_f);
		putc('"', log_f);
	}
	else {
		fprintf(log_f, "%#g", max);
	}
	fputs("]\n", log_f);
}
//
// Recursively print definitions of function referenced.
//
static int print_function_definitions_recursion(at_type * at, int * count, int maxfun, char * definitions[], int depth, int maxdepth)
{
	if(at->a_count == 0)
		return 0;
	else if(*count == maxfun) // limit the maximum number of unique function definitions  
		return 1;
	else if(depth >= maxdepth) // limit the maximum recursion depth 
		return 2;
	else {
		int rc = 0;
		for(int i = 0; (i < at->a_count) && (*count < maxfun); i++) {
			if(oneof2(at->actions[i].index, CALL, CALLN) && at->actions[i].arg.udf_arg->definition) {
				for(int k = 0; k < maxfun; k++) {
					if(definitions[k] == at->actions[i].arg.udf_arg->definition)
						break; // duplicate definition already in list 
					if(definitions[k] == NULL) {
						*count += 1; // increment counter 
						definitions[k] = at->actions[i].arg.udf_arg->definition;
						break;
					}
				}
				rc |= print_function_definitions_recursion(at->actions[i].arg.udf_arg->at, count, maxfun, definitions, depth + 1, maxdepth);
			}
		}
		return rc;
	}
}

static void print_function_definitions(struct at_type * at, FILE * device)
{
	char * definitions[32];
	const int maxfun   = 32; // maximum number of unique functions definitions 
	const int maxdepth = 20; // maximum recursion depth 
	int count = 0;
	memzero(definitions, maxfun * sizeof(char *));
	int rc = print_function_definitions_recursion(at, &count, maxfun, definitions, 0, maxdepth);
	for(int k = 0; k < count; k++)
		fprintf(device, "\t%s\n", definitions[k]);
	if((rc & 1) != 0)
		fprintf(device, "\t[omitting further function definitions (max=%i)]\n", maxfun);
	if((rc & 2) != 0)
		fprintf(device, "\t[too many nested (or recursive) function definitions (max=%i)]\n", maxdepth);
}
// 
// Interface to the gnuplot "fit" command
// 
//void fit_command()
void GnuPlot::FitCommand(GpFit & rFit)
{
	// Backwards compatibility - these were the default names in 4.4 and 4.6	
	static const char * dummy_old_default[5] = {"x", "y", "t", "u", "v"};
	static const int iz = MAX_NUM_VAR;
	// Keep range info in local storage rather than overwriting axis structure.	
	// The final range is "z" (actually the range of the function value).	
	double range_min[MAX_NUM_VAR+1];
	double range_max[MAX_NUM_VAR+1];
	int    range_autoscale[MAX_NUM_VAR+1];
	int    num_ranges = 0;
	int    max_data;
	int    max_params;
	int    dummy_token[MAX_NUM_VAR+1]; // Point to variable name for each explicit range 
	int    skipped[MAX_NUM_VAR+1]; // number of points out of range 
	int    num_points = 0; // number of data points read from file 
	int    i;
	int    j;
	double v[MAX_NUM_VAR+2];
	double tmpd;
	time_t timer;
	int    token1;
	int    token2;
	int    token3;
	char * tmp;
	char * file_name;
	bool   zero_initial_value;
	AxS.Idx_X = FIRST_X_AXIS;
	AxS.Idx_Y = FIRST_Y_AXIS;
	AxS.Idx_Z = FIRST_Z_AXIS;
	GpAxis * fit_xaxis = &AxS[FIRST_X_AXIS];
	GpAxis * fit_yaxis = &AxS[FIRST_Y_AXIS];
	GpAxis * fit_zaxis = &AxS[FIRST_Z_AXIS];
	const int fit_token = Pgm.Shift();
	// First look for a restricted fit range... 
	// Start with the current range limits on variable 1 ("x"),
	// variable 2 ("y"), and function range ("z").
	// Historically variables 3-5 inherited the current range of t, u, and v
	// but no longer.  NB: THIS IS A CHANGE
	fit_xaxis->Init(false);
	fit_yaxis->Init(false);
	fit_zaxis->Init(true);
	for(i = 0; i < MAX_NUM_VAR+1; i++)
		dummy_token[i] = -1;
	range_min[0] = fit_xaxis->Range.low;
	range_max[0] = fit_xaxis->Range.upp;
	range_autoscale[0] = fit_xaxis->autoscale;
	range_min[1] = fit_yaxis->Range.low;
	range_max[1] = fit_yaxis->Range.upp;
	range_autoscale[1] = fit_yaxis->autoscale;
	for(i = 2; i < MAX_NUM_VAR; i++) {
		range_min[i] = VERYLARGE;
		range_max[i] = -VERYLARGE;
		range_autoscale[i] = AUTOSCALE_BOTH;
	}
	range_min[iz] = fit_zaxis->Range.low;
	range_max[iz] = fit_zaxis->Range.upp;
	range_autoscale[iz] = fit_zaxis->autoscale;
	num_ranges = 0;
	while(Pgm.EqualsCur("[")) {
		GpAxis * scratch_axis = &AxS[SAMPLE_AXIS];
		if(i > MAX_NUM_VAR)
			Eexc(Pgm.GetCurTokenIdx(), "too many range specifiers");
		scratch_axis->Init(true);
		scratch_axis->linked_to_primary = NULL;
		dummy_token[num_ranges] = ParseRange((AXIS_INDEX)scratch_axis->index);
		range_min[num_ranges] = scratch_axis->Range.low;
		range_max[num_ranges] = scratch_axis->Range.upp;
		range_autoscale[num_ranges] = scratch_axis->autoscale;
		num_ranges++;
	}
	// now compile the function 
	token1 = Pgm.GetCurTokenIdx();
	if(rFit.func.at) {
		free_at(rFit.func.at);
		rFit.func.at = NULL; /* in case perm_at() does int_error */
	}
	Pgm.dummy_func = &rFit.func;
	// set all possible dummy variable names, even if we're using fewer 
	for(i = 0; i < MAX_NUM_VAR; i++) {
		if(dummy_token[i] > 0)
			Pgm.CopyStr(_Pb.c_dummy_var[i], dummy_token[i], MAX_ID_LEN);
		else if(*_Pb.set_dummy_var[i] != '\0')
			strcpy(_Pb.c_dummy_var[i], _Pb.set_dummy_var[i]);
		else if(i < 5) /* Fall back to legacy ordering x y t u v */
			strcpy(_Pb.c_dummy_var[i], dummy_old_default[i]);
		rFit.fit_dummy_udvs[i] = Ev.AddUdvByName(_Pb.c_dummy_var[i]);
	}
	memzero(_Pb.fit_dummy_var, sizeof(_Pb.fit_dummy_var));
	rFit.func.at = PermAt(); /* parse expression and save action table */
	Pgm.dummy_func = NULL;
	token2 = Pgm.GetCurTokenIdx();
	// get filename 
	{
		const char * p_fn = StringOrExpress(NULL);
		if(p_fn)
			file_name = sstrdup(p_fn);
		else
			Eexc(token2, "missing filename or datablock");
	}
	// We accept a datablock but not a voxel grid 
	if(*file_name == '$' && !GetDatablock(file_name))
		IntError(Pgm.GetPrevTokenIdx(), "cannot fit voxel data");
	// use datafile module to parse the datafile and qualifiers 
	DfSetPlotMode(MODE_QUERY); /* Does nothing except for binary datafiles */
	// Historically we could only handle 7 using specs, hence 5 independent	
	// variables (the last 2 cols are used for z and z_err).
	// June 2013 - Now the number of using specs can be increased by changing
	// MAXDATACOLS.  Logically this should be at least as large as MAX_NUM_VAR,
	// the limit on parameters passed to a user-defined function.
	// I.e. we expect that MAXDATACOLS >= MAX_NUM_VAR + 2
	rFit.columns = DfOpen(_Df, file_name, MAX_NUM_VAR+2, NULL);
	if(rFit.columns < 0)
		Eexc2(token2, "Can't read data from", file_name);
	SAlloc::F(file_name);
	if(rFit.columns == 1)
		Eexc(Pgm.GetCurTokenIdx(), "Need more than 1 input data column");
	// Allow time data only on first two dimensions (x and y) 
	_Df.df_axis[0] = FIRST_X_AXIS;
	_Df.df_axis[1] = FIRST_Y_AXIS;
	// BM: New options to distinguish fits with and without errors 
	// reset error columns 
	memzero(rFit.err_cols, sizeof(rFit.err_cols));
	if(Pgm.AlmostEqualsCur("err$ors")) {
		// error column specs follow 
		Pgm.Shift();
		rFit.num_errors = 0;
		do {
			char * err_spec = NULL;
			if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()))
				Eexc(Pgm.GetCurTokenIdx(), "Expecting a variable specifier.");
			Pgm.MCapture(&err_spec, Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx());
			// check if this is a valid dummy var 
			for(i = 0; i < MAX_NUM_VAR; i++) {
				if(sstreq(err_spec, _Pb.c_dummy_var[i])) {
					rFit.err_cols[i] = TRUE;
					rFit.num_errors++;
					break;
				}
			}
			if(i == MAX_NUM_VAR) { /* variable name not found, yet */
				if(sstreq(err_spec, "z")) {
					rFit.err_cols[iz] = TRUE;
					rFit.num_errors++;
				}
				else
					Eexc(Pgm.GetCurTokenIdx(), "Invalid variable specifier.");
			}
			FPRINTF((stderr, "error spec \"%s\"\n", err_spec));
			SAlloc::F(err_spec);
		} while(Pgm.Equals(++Pgm.CToken, ",") && ++Pgm.CToken);
		// z-errors are required. 
		if(!rFit.err_cols[iz]) {
			Eexc(Pgm.GetCurTokenIdx(), "z-errors are required.");
			rFit.err_cols[iz] = TRUE;
			rFit.num_errors++;
		}
		// The dummy variable with the highest index indicates the minimum number of indep. variables required. 
		rFit.num_indep = 0;
		for(i = 0; i < MAX_NUM_VAR; i++) {
			if(rFit.err_cols[i])
				rFit.num_indep = i + 1;
		}
		// Check if there are enough columns. Require # of indep. and dependent variables + # of errors 
		if(rFit.columns && (rFit.columns < rFit.num_indep + 1 + rFit.num_errors))
			Eexc2(Pgm.GetCurTokenIdx(), "Not enough columns in using spec.  At least %i are required for this error spec.", rFit.num_indep + 1 + rFit.num_errors);
		// Success. 
		if(rFit.columns > 0)
			rFit.num_indep = rFit.columns - rFit.num_errors - 1;
	}
	else if(Pgm.AlmostEqualsCur("zerr$ors")) {
		// convenience alias 
		if(rFit.columns == 1)
			Eexc(Pgm.GetCurTokenIdx(), "zerror requires at least 2 columns");
		rFit.num_indep = (rFit.columns == 0) ? 1 : (rFit.columns - 2);
		rFit.num_errors = 1;
		rFit.err_cols[iz] = TRUE;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("yerr$ors")) {
		// convenience alias, x:z:sz (or x:y:sy) 
		if(rFit.columns && rFit.columns != 3)
			Eexc(Pgm.GetCurTokenIdx(), "yerror requires exactly 3 columns");
		rFit.num_indep = 1;
		rFit.num_errors = 1;
		rFit.err_cols[iz] = TRUE;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("xyerr$ors")) {
		// convenience alias, x:z:sx:sz (or x:y:sx:sy) 
		if(rFit.columns != 0 && rFit.columns != 4)
			Eexc(Pgm.GetCurTokenIdx(), "xyerror requires exactly 4 columns");
		rFit.num_indep = 1;
		rFit.num_errors = 2;
		rFit.err_cols[0] = TRUE;
		rFit.err_cols[iz] = TRUE;
		Pgm.Shift();
	}
	else if(Pgm.AlmostEqualsCur("uni$tweights")) {
		// 'unitweights' are the default now. So basically this option is only useful in v4 compatibility mode.
		// no error columns given 
		Pgm.Shift();
		rFit.num_indep = (rFit.columns == 0) ? 1 : (rFit.columns - 1);
		rFit.num_errors = 0;
	}
	else {
		// no error keyword found 
		if(rFit.fit_v4compatible) {
			// using old syntax 
			rFit.num_indep = (rFit.columns < 3) ? 1 : (rFit.columns - 2);
			rFit.num_errors = (rFit.columns < 3) ? 0 : 1;
			if(rFit.num_errors > 0)
				rFit.err_cols[iz] = TRUE;
		}
		else if(rFit.columns >= 3 && _Pb.fit_dummy_var[rFit.columns-2] == 0) {
			IntWarn(NO_CARET, "\n\t> Implied independent variable %s not found in fit function.\n\t> Assuming version 4 syntax with zerror in column %d but no zerror keyword.\n",
			    _Pb.c_dummy_var[rFit.columns-2], rFit.columns);
			rFit.num_indep = rFit.columns - 2;
			rFit.num_errors = 1;
			rFit.err_cols[iz] = TRUE;
		}
		else {
			// default to unitweights 
			rFit.num_indep = (rFit.columns == 0) ? 1 : (rFit.columns - 1);
			rFit.num_errors = 0;
		}
	}
	FPRINTF((stderr, "cmd=%s\n", Pgm.P_InputLine));
	FPRINTF((stderr, "cols=%i indep=%i errors=%i\n", rFit.columns, rFit.num_indep, rFit.num_errors));
	/* HBB 980401: if this is a single-variable fit, we shouldn't have
	 * allowed a variable name specifier for 'y': */
	/* FIXME EAM - Is this relevant any more? */
	if((dummy_token[1] > 0) && (rFit.num_indep == 1))
		Eexc(dummy_token[1], "Can't re-name 'y' in a one-variable fit");
	// depending on number of independent variables, the last range spec may be for the Z axis 
	if(num_ranges > rFit.num_indep+1)
		Eexc2(dummy_token[num_ranges-1], "Too many range-specs for a %d-variable fit", rFit.num_indep);
	if(num_ranges == (rFit.num_indep + 1)) {
		// last range was actually for the independen variable 
		range_min[iz] = range_min[rFit.num_indep];
		range_max[iz] = range_max[rFit.num_indep];
		range_autoscale[iz] = range_autoscale[rFit.num_indep];
	}
	// defer actually reading the data until we have parsed the rest of the line 
	token3 = Pgm.GetCurTokenIdx();
	// open logfile before we use any Dblfn calls 
	if(!rFit.fit_suppress_log) {
		char * logfile = GetFitLogFile();
		if(logfile && !rFit.log_f && !(rFit.log_f = fopen(logfile, "a")))
			Eex2("could not open log-file %s", logfile);
		SAlloc::F(logfile);
	}
	tmpd = GetDVar(FITLIMIT); // get epsilon if given explicitly 
	rFit.epsilon = (tmpd < 1.0 && tmpd > 0.0) ? tmpd : DEF_FIT_LIMIT;
	FPRINTF((STANDARD, "epsilon=%e\n", rFit.epsilon));
	// tsm patchset 230: new absolute convergence variable 
	FPRINTF((STANDARD, "epsilon_abs=%e\n", rFit.epsilon_abs));
	// maximum number of iterations 
	rFit.maxiter = GetIVar(FITMAXITER);
	if(rFit.maxiter < 0)
		rFit.maxiter = 0;
	FPRINTF((STANDARD, "maxiter=%i\n", rFit.maxiter));
	// get startup value for lambda, if given 
	tmpd = GetDVar(FITSTARTLAMBDA);
	if(tmpd > 0.0) {
		rFit.startup_lambda = tmpd;
		rFit.Dblfn("lambda start value set: %g\n", rFit.startup_lambda);
	}
	else
		rFit.startup_lambda = 0.0; // use default value or calculation 
	// get lambda up/down factor, if given 
	tmpd = GetDVar(FITLAMBDAFACTOR);
	if(tmpd > 0.0) {
		rFit.lambda_up_factor = rFit.lambda_down_factor = tmpd;
		rFit.Dblfn("lambda scaling factors reset:  %g\n", rFit.lambda_up_factor);
	}
	else {
		rFit.lambda_down_factor = LAMBDA_DOWN_FACTOR;
		rFit.lambda_up_factor = LAMBDA_UP_FACTOR;
	}
	FPRINTF((STANDARD, "prescale=%i\n", rFit.fit_prescale));
	FPRINTF((STANDARD, "errorscaling=%i\n", rFit.fit_errorscaling));
	time(&timer);
	if(!rFit.fit_suppress_log) {
		char * line = NULL;
		fputs("\n\n*******************************************************************************\n", rFit.log_f);
		fprintf(rFit.log_f, "%s\n\n", ctime((const time_t * const)&timer));
		Pgm.MCapture(&line, token2, token3 - 1);
		fprintf(rFit.log_f, "FIT:    data read from %s\n", line);
		fprintf(rFit.log_f, "        format = ");
		SAlloc::F(line);
		for(i = 0; (i < rFit.num_indep) && (i < rFit.columns - 1); i++)
			fprintf(rFit.log_f, "%s:", _Pb.c_dummy_var[i]);
		fprintf(rFit.log_f, "z");
		if(rFit.num_errors > 0) {
			for(i = 0; (i < rFit.num_indep) && (i < rFit.columns - 1); i++)
				if(rFit.err_cols[i])
					fprintf(rFit.log_f, ":s%s", _Pb.c_dummy_var[i]);
			fprintf(rFit.log_f, ":s\n");
		}
		else {
			fprintf(rFit.log_f, "\n");
		}
	}
	// report all range specs, starting with Z 
	if(!rFit.fit_suppress_log) {
		if((range_autoscale[iz] & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
			LogAxisRestriction(rFit.log_f, iz, range_min[iz], range_max[iz], range_autoscale[iz], "function");
		for(i = 0; i < rFit.num_indep; i++) {
			if((range_autoscale[i] & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
				LogAxisRestriction(rFit.log_f, i, range_min[i], range_max[i], range_autoscale[i], _Pb.c_dummy_var[i]);
		}
	}
	// start by allocting memory for MAX_DATA datapoints 
	max_data = MAX_DATA;
	rFit.fit_x = vec(max_data * rFit.num_indep);
	rFit.fit_z = vec(max_data);
	// allocate error array, last one is always the z-error 
	rFit.err_data = vec(max_data * MAX(rFit.num_errors, 1));
	rFit.num_data = 0;
	// Set skipped[i] = 0 for all i 
	memzero(skipped, sizeof(skipped));
	// first read in experimental data 

	// If the user has set an explicit locale for numeric input, apply it 
	// here so that it affects data fields read from the input file.      
	set_numeric_locale();
	while((i = DfReadLine(_Df, v, rFit.num_indep + rFit.num_errors + 1)) != DF_EOF) {
		if(rFit.num_data >= max_data) {
			max_data *= 2;
			if(!redim_vec(&rFit.fit_x, max_data * rFit.num_indep) || !redim_vec(&rFit.fit_z, max_data) || !redim_vec(&rFit.err_data, max_data * MAX(rFit.num_errors, 1))) {
				// Some of the reallocations went bad: 
				Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
			}
		} /* if (need to extend storage space) */
		// BM: silently ignore lines with NaN 
		{
			bool skip_nan = FALSE;
			for(int k = 0; k < i; k++) {
				if(isnan(v[k]))
					skip_nan = TRUE;
			}
			if(skip_nan)
				continue;
		}
		switch(i) {
			case DF_MISSING:
			case DF_UNDEFINED:
			case DF_FIRST_BLANK:
			case DF_SECOND_BLANK:
			    continue;
			case DF_COLUMN_HEADERS:
			    continue;
			case DF_FOUND_KEY_TITLE:
			    continue;
			case 0:
			    Eex2("bad data on line %d of datafile", _Df.df_line_number);
			    break;
			case 1: /* only z provided */
			    v[1] = v[0];
			    v[0] = (double)_Df.df_datum;
			    break;
			default: /* June 2013 - allow more than 7 data columns */
			    if(i<0)
				    Eex("unexpected value returned by df_readline");
			    break;
		}
		num_points++;
		// skip this point if it is out of range 
		for(i = 0; i < rFit.num_indep; i++) {
			if(!(range_autoscale[i] & AUTOSCALE_MIN) && (v[i] < range_min[i])) {
				skipped[i]++;
				goto out_of_range;
			}
			if(!(range_autoscale[i] & AUTOSCALE_MAX) && (v[i] > range_max[i])) {
				skipped[i]++;
				goto out_of_range;
			}
			rFit.fit_x[rFit.num_data * rFit.num_indep + i] = v[i]; // save independent variable data 
		}
		// check Z value too 
		if(!(range_autoscale[iz] & AUTOSCALE_MIN) && (v[i] < range_min[iz])) {
			skipped[iz]++;
			goto out_of_range;
		}
		if(!(range_autoscale[iz] & AUTOSCALE_MAX) && (v[i] > range_max[iz])) {
			skipped[iz]++;
			goto out_of_range;
		}
		rFit.fit_z[rFit.num_data] = v[i++]; // save dependent variable data 
		// only use error from data file if _explicitly_ asked for by a using spec 
		if(rFit.num_errors == 0)
			rFit.err_data[rFit.num_data] = 1.0; /* constant weight */
		else if(rFit.num_errors == 1)
			rFit.err_data[rFit.num_data] = v[i++]; /* z-error */
		else {
			int idx = 0;
			for(int k = 0; k < MAX_NUM_VAR; k++) {
				if(rFit.err_cols[k])
					rFit.err_data[rFit.num_errors * rFit.num_data + idx++] = v[i++];
			}
			if(rFit.err_cols[iz])
				rFit.err_data[rFit.num_errors * rFit.num_data + idx] = v[i++]; /* z-error */
			else
				Eexc(NO_CARET, "z errors are always required"); // This case is not currently allowed. We always require z-errors. 
		}
		// Increment index into stored values.
		// Note that out-of-range or NaN values bypass this operation.
		rFit.num_data++;
out_of_range:
		;
	}
	DfClose(_Df);
	// We are finished reading user input; return to C locale for internal use 
	reset_numeric_locale();
	if(rFit.num_data <= 1) {
		// no data! Try to explain why. 
		printf("         Read %d points\n", num_points);
		for(i = 0; i < rFit.num_indep; i++) {
			if(skipped[i]) {
				printf("         Skipped %d points outside range [%s=", skipped[i], _Pb.c_dummy_var[i]);
				if(range_autoscale[i] & AUTOSCALE_MIN)
					printf("*:");
				else
					printf("%g:", range_min[i]);
				if(range_autoscale[i] & AUTOSCALE_MAX)
					printf("*]\n");
				else
					printf("%g]\n", range_max[i]);
			}
		}
		if(skipped[iz]) {
			printf("         Skipped %d points outside range [%s=", skipped[iz], "z");
			if(AxS[FIRST_Z_AXIS].autoscale & AUTOSCALE_MIN)
				printf("*:");
			else
				printf("%g:", AxS[FIRST_Z_AXIS].Range.low);
			if(AxS[FIRST_Z_AXIS].autoscale & AUTOSCALE_MAX)
				printf("*]\n");
			else
				printf("%g]\n", AxS[FIRST_Z_AXIS].Range.upp);
		}
		Eex("No data to fit");
	}
	// tsm patchset 230: check for zero error values 
	if(rFit.num_errors > 0) {
		for(i = 0; i < rFit.num_data; i++) {
			if(rFit.err_data[i * rFit.num_errors + (rFit.num_errors - 1)] != 0.0)
				continue;
			rFit.Dblfn("\nCurrent data point\n");
			rFit.Dblfn("=========================\n");
			rFit.Dblfn("%-15s = %i out of %i\n", "#", i + 1, rFit.num_data);
			for(j = 0; j < rFit.num_indep; j++)
				rFit.Dblfn("%-15.15s = %-15g\n", _Pb.c_dummy_var[j], rFit.fit_x[i * rFit.num_indep + j]);
			rFit.Dblfn("%-15.15s = %-15g\n", "z", rFit.fit_z[i]);
			rFit.Dblfn("%-15.15s = %-15g\n", "s", rFit.err_data[i * rFit.num_errors + (rFit.num_errors - 1)]);
			rFit.Dblfn("\n");
			Eex("Zero error value in data file");
		}
	}
	// now resize fields to actual length: 
	redim_vec(&rFit.fit_x, rFit.num_data * rFit.num_indep);
	redim_vec(&rFit.fit_z, rFit.num_data);
	redim_vec(&rFit.err_data, rFit.num_data * MAX(rFit.num_errors, 1));
	if(!rFit.fit_suppress_log) {
		char * line = NULL;
		fprintf(rFit.log_f, "        #datapoints = %d\n", rFit.num_data);
		if(rFit.num_errors == 0)
			fputs("        residuals are weighted equally (unit weight)\n\n", rFit.log_f);
		Pgm.MCapture(&line, token1, token2 - 1);
		fprintf(rFit.log_f, "function used for fitting: %s\n", line);
		print_function_definitions(rFit.func.at, rFit.log_f);
		SAlloc::F(line);
	}
	// read in parameters 
	max_params = MAX_PARAMS;
	if(!Pgm.EqualsCurShift("via"))
		Eexc(Pgm.GetCurTokenIdx(), "Need via and either parameter list or file");
	// allocate arrays for parameter values, names 
	rFit.a = vec(max_params);
	rFit.par_name = (fixstr*)SAlloc::M((max_params + 1) * sizeof(fixstr));
	rFit.par_udv = (GpValue **)SAlloc::R(rFit.par_udv, (max_params + 1) * sizeof(GpValue *));
	rFit.num_params = 0;
	// 
	// FIXME: This is all done by character-by-character inspection of the
	// input line.  If it were wrapped in lf_push()/lf_pop() we could use
	// the normal gnuplot parsing routines keyed off c_token.
	// 
	if(IsStringValue(Pgm.GetCurTokenIdx())) {    /* It's a parameter *file* */
		bool fixed;
		double tmp_par;
		char c = '\0', * s;
		char sstr[MAX_LINE_LEN + 1];
		char * viafile = TryToGetString();
		if(!viafile || !(rFit.via_f = LoadPath_fopen(viafile, "r")))
			Eex2("could not read parameter-file \"%s\"", viafile);
		if(!rFit.fit_suppress_log)
			fprintf(rFit.log_f, "fitted parameters and initial values from file: %s\n\n", viafile);
		SAlloc::F(viafile); /* Free previous name, if any */
		// get parameters and values out of file and ignore fixed ones 
		while(TRUE) {
			if(!fgets(s = sstr, sizeof(sstr), rFit.via_f)) // EOF found 
				break;
			if((tmp = strstr(s, GP_FIXED)) != NULL) { /* ignore fixed params */
				*tmp = '\0';
				if(!rFit.fit_suppress_log)
					fprintf(rFit.log_f, "FIXED:  %s\n", s);
				fixed = TRUE;
			}
			else
				fixed = FALSE;
			if((tmp = sstrchr(s, '#')) != NULL)
				*tmp = '\0';
			if(!GpFit::IsEmptyString(s)) {
				tmp = get_next_word(&s, &c);
				if(!legal_identifier(tmp) || strlen(tmp) > MAX_ID_LEN)
					Eex("syntax error in parameter file");
				if(c == '[') {
					// Special case: array element 
					udvt_entry * udv = Ev.GetUdvByName(tmp);
					int index;
					if(udv->udv_value.Type != ARRAY)
						Eex("no such array");
					if((1 != sscanf(s, "%d]", &index)) || (index <= 0 || index > udv->udv_value.v.value_array[0].v.int_val))
						Eex("bad array index");
					snprintf(rFit.par_name[rFit.num_params], sizeof(rFit.par_name[0]), "%40.40s[%d]", tmp, (short)index);
					rFit.par_udv[rFit.num_params] = &(udv->udv_value.v.value_array[index]);
				}
				else {
					// Normal case 
					strnzcpy(rFit.par_name[rFit.num_params], tmp, sizeof(fixstr));
					rFit.par_udv[rFit.num_params] = &Ev.AddUdvByName(tmp)->udv_value;
				}
				// next must be '=' 
				if(c != '=') {
					tmp = sstrchr(s, '=');
					if(!tmp)
						Eex("syntax error in parameter file");
					s = tmp + 1;
				}
				tmp = get_next_word(&s, &c);
				if(sscanf(tmp, "%lf", &tmp_par) != 1)
					Eex("syntax error in parameter file");
				Gcomplex(rFit.par_udv[rFit.num_params], tmp_par, 0.0);
				// Fixed parameters are updated but not counted against num_params 
				if(!fixed) {
					if(rFit.num_params >= max_params)
						Eex("too many fit parameters");
					rFit.a[rFit.num_params++] = tmp_par;
				}
				if((tmp = get_next_word(&s, &c)) != NULL)
					Eex("syntax error in parameter file");
			}
		}
		SFile::ZClose(&rFit.via_f);
	}
	else {
		// not a string after via: it's a variable listing 
		if(!rFit.fit_suppress_log)
			fputs("fitted parameters initialized with current variable values\n\n", rFit.log_f);
		do {
			if(!Pgm.IsLetter(Pgm.GetCurTokenIdx()))
				Eex("no parameter specified");
			if(rFit.num_params >= max_params)
				Eex("too many fit parameters");
			if(Pgm.EqualsNext("[")) {
				// Special case:  via Array[n] created variables will be of the form Array_n_*
				udvt_entry * udv = AddUdv(Pgm.GetCurTokenIdx());
				int index;
				if(udv->udv_value.Type != ARRAY)
					Eexc(Pgm.GetCurTokenIdx(), "No such array");
				Pgm.Shift();
				Pgm.Shift();
				index = IntExpression();
				if(index <= 0 || index > udv->udv_value.v.value_array[0].v.int_val)
					Eexc(Pgm.GetCurTokenIdx(), "array index out of range");
				if(!Pgm.EqualsCur("]"))
					Eexc(Pgm.GetCurTokenIdx(), "not an array index");
				snprintf(rFit.par_name[rFit.num_params], sizeof(rFit.par_name[0]), "%s[%d]", udv->udv_name, index);
				rFit.a[rFit.num_params] = Real(&(udv->udv_value.v.value_array[index]) );
				rFit.par_udv[rFit.num_params] = &(udv->udv_value.v.value_array[index]);
			}
			else {
				// Normal case: via param_name 
				Pgm.Capture(rFit.par_name[rFit.num_params], Pgm.GetCurTokenIdx(), Pgm.GetCurTokenIdx(), (int)sizeof(rFit.par_name[0]));
				// create variable if it doesn't exist 
				rFit.a[rFit.num_params] = CreateDVar(rFit.par_name[rFit.num_params], INITIAL_VALUE);
				rFit.par_udv[rFit.num_params] = &Ev.GetUdvByName(rFit.par_name[rFit.num_params])->udv_value;
			}
			rFit.num_params++;
		} while(Pgm.Equals(++Pgm.CToken, ",") && ++Pgm.CToken);
	}
	redim_vec(&rFit.a, rFit.num_params);
	rFit.par_name = (fixstr*)SAlloc::R(rFit.par_name, (rFit.num_params + 1) * sizeof(fixstr));
	if(rFit.num_data < rFit.num_params)
		Eex("Number of data points smaller than number of parameters");
	// initialize scaling parameters 
	if(!redim_vec(&rFit.scale_params, rFit.num_params))
		Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
	zero_initial_value = FALSE;
	for(i = 0; i < rFit.num_params; i++) {
		// avoid parameters being equal to zero 
		if(rFit.a[i] == 0.0) {
			rFit.Dblfn("Warning: Initial value of parameter '%s' is zero.\n", rFit.par_name[i]);
			rFit.a[i] = NEARLY_ZERO;
			rFit.scale_params[i] = 1.0;
			zero_initial_value = TRUE;
		}
		else if(rFit.fit_prescale) {
			// scale parameters, but preserve sign 
			double a_sign = (rFit.a[i] > 0) - (rFit.a[i] < 0);
			rFit.scale_params[i] = a_sign * rFit.a[i];
			rFit.a[i] = a_sign;
		}
		else {
			rFit.scale_params[i] = 1.0;
		}
	}
	if(zero_initial_value) { // print this message only once 
		// tsm patchset 230: explain what good initial parameter values are 
		fprintf(STANDARD, "  Please provide non-zero initial values for the parameters, at least of\n");
		fprintf(STANDARD, "  the right order of magnitude. If the expected value is zero, then use\n");
		fprintf(STANDARD, "  the magnitude of the expected error. If all else fails, try 1.0\n\n");
	}
	if(rFit.num_params == 0)
		IntWarn(NO_CARET, "No fittable parameters!\n");
	else
		Regress(rFit, rFit.a); // fit 
	SFile::ZClose(&rFit.log_f);
	ZFREE(rFit.fit_x);
	ZFREE(rFit.fit_z);
	ZFREE(rFit.err_data);
	ZFREE(rFit.a);
	if(rFit.func.at) {
		free_at(rFit.func.at); // release perm. action table 
		rFit.func.at = (at_type *)NULL;
	}
	// remember parameter names for 'update' 
	rFit.last_num_params = rFit.num_params;
	FREEANDASSIGN(rFit.last_par_name, rFit.par_name);
	// remember names of indep. variables for 'update' 
	for(i = 0; i < MAX_NUM_VAR; i++) {
		FREEANDASSIGN(rFit.last_dummy_var[i], sstrdup(_Pb.c_dummy_var[i]));
	}
	// remember last fit command for 'save fit' 
	// FIXME: This breaks if there is a ; internal to the fit command 
	FREEANDASSIGN(rFit.last_fit_command, sstrdup(&Pgm.P_InputLine[Pgm.P_Token[fit_token].StartIdx]));
	if(sstrchr(rFit.last_fit_command, ';'))
		*sstrchr(rFit.last_fit_command, ';') = '\0';
	// save fit command to user variable 
	Ev.FillGpValString("GPVAL_LAST_FIT", rFit.last_fit_command);
}
// 
// Print message to stderr and log file
// 
void GnuPlot::GpFit::Dblfn(const char * fmt, ...) const
{
	va_list args;
	VA_START(args, fmt);
	if(fit_verbosity != QUIET)
		vfprintf(STANDARD, fmt, args);
	va_end(args);
	if(!fit_suppress_log) {
		VA_START(args, fmt);
		vfprintf(log_f, fmt, args);
	}
	va_end(args);
}
//
// Get name of current log-file
//
//char * getfitlogfile()
char * GnuPlot::GetFitLogFile()
{
	char * logfile = NULL;
	if(!_Fit.fitlogfile) {
		char * tmp = getenv(GNUFITLOG); /* open logfile */
		// If GNUFITLOG is defined but null, do not write to log file 
		if(tmp && *tmp == '\0') {
			_Fit.fit_suppress_log = TRUE;
			return NULL;
		}
		if(!isempty(tmp)) {
			char * tmp2 = tmp + (strlen(tmp) - 1);
			// if given log file name ends in path separator, treat it as a directory to store the default "fit.log" in 
			if(oneof2(*tmp2, '/', '\\')) {
				logfile = (char *)SAlloc::M(strlen(tmp) + strlen(fitlogfile_default) + 1);
				strcpy(logfile, tmp);
				strcat(logfile, fitlogfile_default);
			}
			else
				logfile = sstrdup(tmp);
		}
		else
			logfile = sstrdup(fitlogfile_default);
	}
	else
		logfile = sstrdup(_Fit.fitlogfile);
	return logfile;
}
// 
// replacement for "update", which is now deprecated.
// write current value of parameters used in previous fit to a file.
// That file can be used as an argument to 'via' in a subsequent fit command.
// 
//void save_fit(FILE * fp)
void GnuPlot::SaveFit(const GpFit & rFit, FILE * fp)
{
	if(isempty(rFit.last_fit_command)) {
		IntWarn(NO_CARET, "no previous fit command");
	}
	else {
		fputs("# ", fp);
		fputs(rFit.last_fit_command, fp);
		fputs("\n", fp);
		udvt_entry * udv = Ev.GetUdvByName("FIT_STDFIT");
		if(udv)
			fprintf(fp, "# final sum of squares of residuals : %g\n", udv->udv_value.v.cmplx_val.real);
		for(int k = 0; k < rFit.last_num_params; k++)
			fprintf(fp, "%-15s = %-22s\n", rFit.last_par_name[k], ValueToStr(rFit.par_udv[k], FALSE));
	}
}
