/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *    Nonlinear least squares fit according to the
 *      Marquardt-Levenberg-algorithm
 *
 *      added as Patch to Gnuplot (v3.2 and higher)
 *      by Carsten Grammes
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
 * Lars Hecking : review update command, for VMS in particular, where
 * it is not necessary to rename the old file.
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
 * x and y still refer to GpGg[] in order to allow time/date formats.
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
//#include "standard.h"
//#include "specfun.h"
//#include <signal.h>

GpFit GpF; // @global
//
// Just temporary
//
#define Dblf  Dblfn
#define Dblf2 Dblfn
#define Dblf3 Dblfn
#define Dblf5 Dblfn
#define Dblf6 Dblfn
#if defined(MSDOS)      /* non-blocking IO stuff */
	#include <io.h>
	#include <conio.h>
	#include <dos.h>
#elif !defined(VMS)
	#include <fcntl.h>
#endif
#ifdef WIN32
	#include "win/winmain.h"
#endif

/* constants */

#ifdef INFINITY
	#undef INFINITY
#endif

#define INFINITY    1e30
#define NEARLY_ZERO 1e-30
#define INITIAL_VALUE 1.0 /* create new variables with this value (was NEARLY_ZERO) */
#define DELTA       0.001 /* Relative change for derivatives */
#define MAX_DATA    2048
#define MAX_PARAMS  32
#define MAX_LAMBDA  1e20
#define MIN_LAMBDA  1e-20
#define LASTFITCMDLENGTH 511
#define STANDARD stderr /* compatible with gnuplot philosophy */
#define BACKUP_SUFFIX ".old" /* Suffix of a backup file */
#define SQR(x) ((x) * (x))
//
// internal Prototypes
//
#if !defined(WIN32) || defined(WGP_CONSOLE)
	static RETSIGTYPE ctrlc_handle(int an_int);
#endif
static void ctrlc_setup();
static void log_axis_restriction(FILE *log_f, int param, double min, double max, int autoscale, char * name);
static void print_function_definitions(AtType * at, FILE * device);
static bool is_empty(char* s);
static int getivar(const char* varname);
static double getdvar(const char* varname);
static double createdvar(char * varname, double value);
static void setvar(char * varname, double value);
static void setvarerr(char * varname, double value);
static void setvarcovar(char * varname1, char * varname2, double value);
static char * get_next_word(char ** s, char * subst);
//
// Small function to write the last fit command into a file
// Arg: Pointer to the file; if NULL, nothing is written,
//   but only the size of the string is returned.
//
size_t GpFit::WriToFilLastFitCmd(FILE * fp)
{
	if(last_fit_command == NULL)
		return 0;
	if(fp == NULL)
		return strlen(last_fit_command);
	else
		return (size_t)fputs(last_fit_command, fp);
}

/*****************************************************************
    This is called when a SIGINT occurs during fit
*****************************************************************/

#if !defined(WIN32) || defined(WGP_CONSOLE)
static RETSIGTYPE ctrlc_handle(int an_int)
{
	(void)an_int;           
	/* reinstall signal handler (necessary on SysV) */
	signal(SIGINT, (sigfunc)ctrlc_handle);
	ctrlc_flag = true;
}

#endif

/*****************************************************************
    setup the ctrl_c signal handler
*****************************************************************/
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
#if(defined(__EMX__) || !defined(MSDOS)) && (!defined(WIN32) || defined(WGP_CONSOLE))
	signal(SIGINT, (sigfunc)ctrlc_handle);
#endif
}

//
// in case of fatal errors
//
//void error_ex(int t_num, const char * str, ...)
void GpFit::ErrorEx(int t_num, const char * str, ...)
{
	char buf[128];
	va_list args;
	va_start(args, str);
	vsnprintf(buf, sizeof(buf), str, args);
	va_end(args);
	// cleanup - free memory
	if(log_f) {
		fprintf(log_f, "BREAK: %s", buf);
		fclose(log_f);
		log_f = NULL;
	}
	ZFREE(fit_x);
	ZFREE(fit_z);
	ZFREE(err_data);
	ZFREE(P_A);
	if(func.at) {
		AtType::Destroy(func.at); // release perm. action table 
		func.at = (AtType*)NULL;
	}
	if(regress_cleanup != NULL)
		(*regress_cleanup)();
	GpDf.DfClose(); // the datafile may still be open
	interrupt_setup(); // restore original SIGINT function
	GpGg.IntError(t_num, buf); // exit via int_error() so that it can clean up state variables
}
//
// Marquardt's nonlinear least squares fit
//
//static marq_res_t marquardt(double a[], double ** C, double * chisq, double * lambda)
GpFit::marq_res_t GpFit::Marquardt(double a[], double ** C, double * chisq, double * lambda)
{
	static double * da = 0; /* delta-step of the parameter */
	static double * temp_a = 0; /* temptative new params set   */
	static double * d = 0;
	static double * tmp_d = 0;
	static double ** tmp_C = 0;
	static double * residues = 0;
	static double ** deriv = 0;
	int    i, j;
	double tmp_chisq;
	/* Initialization when lambda == -1 */
	if(*lambda == -1) {     /* Get first chi-square check */
		temp_a = vec(num_params);
		d = vec(num_data + num_params);
		tmp_d = vec(num_data + num_params);
		da = vec(num_params);
		residues = vec(num_data + num_params);
		tmp_C = matr(num_data + num_params, num_params);
		deriv = NULL;
		if(num_errors > 1)
			deriv = matr(num_errors - 1, num_data);
		Analyze(a, C, d, chisq, deriv);
		// Calculate a useful startup value for lambda, as given by Schwarz
		if(startup_lambda != 0)
			*lambda = startup_lambda;
		else {
			*lambda = 0;
			for(i = 0; i < num_data; i++)
				for(j = 0; j < num_params; j++)
					*lambda += C[i][j] * C[i][j];
			*lambda = sqrt(*lambda / num_data / num_params);
		}
		// Fill in the lower square part of C (the diagonal is filled in on each iteration, see below)
		for(i = 0; i < num_params; i++)
			for(j = 0; j < i; j++) {
				C[num_data + i][j] = 0, C[num_data + j][i] = 0;
			}
		return OK;
	}
	/* once converged, free allocated vars */
	if(*lambda == -2) {
		ZFREE(d);
		ZFREE(tmp_d);
		ZFREE(da);
		ZFREE(temp_a);
		ZFREE(residues);
		free_matr(tmp_C);
		free_matr(deriv);
		tmp_C = deriv = (double**)NULL;
		return OK;
	}
	/* Givens calculates in-place, so make working copies of C and d */
	for(j = 0; j < num_data + num_params; j++)
		memcpy(tmp_C[j], C[j], num_params * sizeof(double));
	memcpy(tmp_d, d, num_data * sizeof(double));
	/* fill in additional parts of tmp_C, tmp_d */
	for(i = 0; i < num_params; i++) {
		/* fill in low diag. of tmp_C ... */
		tmp_C[num_data + i][i] = *lambda;
		/* ... and low part of tmp_d */
		tmp_d[num_data + i] = 0;
	}
	Givens(tmp_C, tmp_d, da, num_params + num_data, num_params);
	/* check if trial did ameliorate sum of squares */
	for(j = 0; j < num_params; j++)
		temp_a[j] = a[j] + da[j];
	Analyze(temp_a, tmp_C, tmp_d, &tmp_chisq, deriv);

	/* tsm patchset 230: Changed < to <= in next line */
	/* so finding exact minimum stops iteration instead of just increasing lambda. */
	/* Disadvantage is that if lambda is large enough so that chisq doesn't change */
	/* is taken as success. */
	if(tmp_chisq <= *chisq) { /* Success, accept new solution */
		if(*lambda > MIN_LAMBDA) {
			if(fit_verbosity == VERBOSE)
				putc('/', stderr);
			*lambda /= lambda_down_factor;
		}
		/* update chisq, C, d, a */
		*chisq = tmp_chisq;
		for(j = 0; j < num_data; j++) {
			memcpy(C[j], tmp_C[j], num_params * sizeof(double));
			d[j] = tmp_d[j];
		}
		for(j = 0; j < num_params; j++)
			a[j] = temp_a[j];
		return BETTER;
	}
	else {                  /* failure, increase lambda and return */
		*lambda *= lambda_up_factor;
		if(fit_verbosity == VERBOSE)
			putc('*', stderr);
		else if(fit_verbosity == BRIEF) /* one-line report even if chisq increases */
			FitShowBrief(-1, tmp_chisq, *chisq, temp_a, *lambda, STANDARD);
		return WORSE;
	}
}

/*****************************************************************
    compute the (effective) error
*****************************************************************/
//static double effective_error(double ** deriv, int i)
double GpFit::EffectiveError(double ** deriv, int i)
{
	double tot_err;
	int j, k;
	if(num_errors <= 1) /* z-errors or equal weights */
		tot_err = err_data[i];
	else {
		// "Effective variance" according to Jay Orear, Am. J. Phys., Vol. 50, No. 10, October 1982
		tot_err = SQR(err_data[i * num_errors + (num_errors - 1)]);
		for(j = 0, k = 0; j < num_indep; j++) {
			if(err_cols[j]) {
				tot_err += SQR(deriv[k][i] * err_data[i * num_errors + k]);
				k++;
			}
		}
		tot_err = sqrt(tot_err);
	}
	return tot_err;
}

/*****************************************************************
    compute chi-square and numeric derivations
*****************************************************************/
//
// Used by marquardt to evaluate the linearized fitting matrix C and
// vector d. Fills in only the top part of C and d. I don't use a
// temporary array zfunc[] any more. Just use d[] instead.
//
//static void analyze(double a[], double ** C, double d[], double * chisq, double ** deriv)
void GpFit::Analyze(double a[], double ** C, double d[], double * chisq, double ** deriv)
{
	int i, j;
	Calculate(d, C, a);
	// derivatives in indep. variables are required for effective variance method
	if(num_errors > 1)
		CalcEerivatives(a, d, deriv);
	for(i = 0; i < num_data; i++) {
		double err = EffectiveError(deriv, i);
		/* note: order reversed, as used by Schwarz */
		d[i] = (d[i] - fit_z[i]) / err;
		for(j = 0; j < num_params; j++)
			C[i][j] /= err;
	}
	*chisq = sumsq_vec(num_data, d);
}

/*****************************************************************
    compute function values and partial derivatives of chi-square
*****************************************************************/
/* To use the more exact, but slower two-side formula, activate the
   following line: */
/*#define TWO_SIDE_DIFFERENTIATION */
//static void calculate(double * zfunc, double ** dzda, double a[])
void GpFit::Calculate(double * zfunc, double ** dzda, double a[])
{
	int    k, p;
	double tmp_a;
	double * tmp_high, * tmp_pars;
#ifdef TWO_SIDE_DIFFERENTIATION
	double * tmp_low;
#endif
	tmp_high = vec(num_data); /* numeric derivations */
#ifdef TWO_SIDE_DIFFERENTIATION
	tmp_low = vec(num_data);
#endif
	tmp_pars = vec(num_params);
	// first function values
	CallGnuplot(a, zfunc);
	// then derivatives in parameters
	for(p = 0; p < num_params; p++)
		tmp_pars[p] = a[p];
	for(p = 0; p < num_params; p++) {
		tmp_a = fabs(a[p]) < NEARLY_ZERO ? NEARLY_ZERO : a[p];
		tmp_pars[p] = tmp_a * (1 + DELTA);
		CallGnuplot(tmp_pars, tmp_high);
#ifdef TWO_SIDE_DIFFERENTIATION
		tmp_pars[p] = tmp_a * (1 - DELTA);
		CallGnuplot(tmp_pars, tmp_low);
#endif
		for(k = 0; k < num_data; k++)
#ifdef TWO_SIDE_DIFFERENTIATION
			dzda[k][p] = (tmp_high[k] - tmp_low[k]) / (2 * tmp_a * DELTA);
#else
			dzda[k][p] = (tmp_high[k] - zfunc[k]) / (tmp_a * DELTA);
#endif
		tmp_pars[p] = a[p];
	}

#ifdef TWO_SIDE_DIFFERENTIATION
	free(tmp_low);
#endif
	free(tmp_high);
	free(tmp_pars);
}
//
// call internal gnuplot functions
//
//void call_gnuplot(const double * par, double * data)
void GpFit::CallGnuplot(const double * par, double * data)
{
	int    i, j;
	t_value v;
	// set parameters first
	for(i = 0; i < num_params; i++)
		setvar(par_name[i], par[i] * scale_params[i]);
	for(i = 0; i < num_data; i++) {
		// calculate fit-function value
		// initialize extra dummy variables from the corresponding actual variables, if any
		for(j = 0; j < MAX_NUM_VAR; j++) {
			UdvtEntry * udv = fit_dummy_udvs[j];
			if(!udv)
				GpGg.IntErrorNoCaret("Internal error: lost a dummy parameter!");
			func.dummy_values[j].SetComplex(udv->udv_value.type == NOTDEFINED ? 0 : udv->udv_value.Real(), 0.0);
		}
		// set actual dummy variables from file data
		for(j = 0; j < num_indep; j++)
			func.dummy_values[j].SetComplex(fit_x[i * num_indep + j], 0.0);
		GpGg.Ev.EvaluateAt(func.at, &v);
		data[i] = v.Real();
		if(GpGg.Ev.undefined || fisnan(data[i])) {
			// Print useful info on undefined-function error
			Dblf("\nCurrent data point\n");
			Dblf("=========================\n");
			Dblf3("%-15s = %i out of %i\n", "#", i + 1, num_data);
			for(j = 0; j < num_indep; j++)
				Dblf3("%-15.15s = %-15g\n", GpGg.Gp__C.P.CDummyVar[j], par[j] * scale_params[j]);
			Dblf3("%-15.15s = %-15g\n", "z", fit_z[i]);
			Dblf("\nCurrent set of parameters\n");
			Dblf("=========================\n");
			for(j = 0; j < num_params; j++)
				Dblf3("%-15.15s = %-15g\n", par_name[j], par[j] * scale_params[j]);
			Dblf("\n");
			if(GpGg.Ev.undefined) {
				Eex("Undefined value during function evaluation");
			}
			else {
				Eex("Function evaluation yields NaN (\"not a number\")");
			}
		}
	}
}

/*****************************************************************
    calculate derivatives wrt the parameters
*****************************************************************/
/* Used to calculate the effective variance in effective_error() */
//static void calc_derivatives(const double * par, double * data, double ** deriv)
void GpFit::CalcEerivatives(const double * par, double * data, double ** deriv)
{
	int i, j, k, m;
	t_value v;
	double h;
	// set parameters first
	for(i = 0; i < num_params; i++)
		setvar(par_name[i], par[i] * scale_params[i]);
	for(i = 0; i < num_data; i++) { /* loop over data points */
		for(j = 0, m = 0; j < num_indep; j++) { /* loop over indep. variables */
			double tmp_high;
			double tmp_x;
#ifdef TWO_SIDE_DIFFERENTIATION
			double tmp_low;
#endif
			// only calculate derivatives if necessary
			if(err_cols[j]) {
				// set actual dummy variables from file data
				for(k = 0; k < num_indep; k++) {
					if(j != k)
						func.dummy_values[k].SetComplex(fit_x[i * num_indep + k], 0.0);
				}
				tmp_x = fit_x[i * num_indep + j];
				// optimal step size
				h = MAX(DELTA * fabs(tmp_x), 8*1e-8*(fabs(tmp_x) + 1e-8));
				func.dummy_values[j].SetComplex(tmp_x + h, 0.0);
				GpGg.Ev.EvaluateAt(func.at, &v);
				tmp_high = v.Real();
#ifdef TWO_SIDE_DIFFERENTIATION
				func.dummy_values[j].SetComplex(tmp_x - h, 0.0);
				GpGg.Ev.EvaluateAt(func.at, &v);
				tmp_low = v.Real();
				deriv[m][i] = (tmp_high - tmp_low) / (2 * h);
#else
				deriv[m][i] = (tmp_high - data[i]) / h;
#endif
				m++;
			}
		}
	}
}

/*****************************************************************
    handle user interrupts during fit
*****************************************************************/
//static bool fit_interrupt()
bool GpFit::FitInterrupt()
{
	while(true) {
		fputs("\n\n(S)top fit, (C)ontinue, (E)xecute FIT_SCRIPT:  ", STANDARD);
#ifdef WIN32
		WinRaiseConsole();
#endif
		switch(getchar()) {
			case EOF:
			case 's':
			case 'S':
			    fputs("Stop.\n", STANDARD);
			    user_stop = true;
			    return false;
			case 'c':
			case 'C':
			    fputs("Continue.\n", STANDARD);
			    return true;
			case 'e':
			case 'E':
				{
					char * tmp = GetFitScript();
					fprintf(STANDARD, "executing: %s\n", tmp);
					// FIXME: Shouldn't we also set FIT_STDFIT etc?
					// set parameters visible to gnuplot
					for(int i = 0; i < num_params; i++)
						setvar(par_name[i], P_A[i] * scale_params[i]);
					GpGg.Gp__C.DoString(tmp);
					free(tmp);
				}
		}
	}
	return true;
}
//
// determine current setting of FIT_SCRIPT
//
//char * getfitscript()
char * GpFit::GetFitScript()
{
	char * tmp;
	if(fit_script)
		return gp_strdup(fit_script);
	else if((tmp = getenv(FITSCRIPT)) != NULL)
		return gp_strdup(tmp);
	else
		return gp_strdup(DEFAULT_CMD);
}
//
// initial setup for regress()
//
//static void regress_init()
void GpFit::RegressInit()
{
	// Reset flag describing fit result status
	UdvtEntry * v = GpGg.Ev.AddUdvByName("FIT_CONVERGED"); // For exporting results to the user
	v->udv_value.SetInt(0);
	// Ctrl-C now serves as Hotkey
	ctrlc_setup();
	// HBB 981118: initialize new variable 'user_break'
	user_stop = false;
}
//
// finalize regression: print results and set user variables
//
//static void regress_finalize(int iter, double chisq, double last_chisq, double lambda, double ** covar)
void GpFit::RegressFinalize(int iter, double chisq, double last_chisq, double lambda, double ** covar)
{
	int i, j;
	UdvtEntry * v;  /* For exporting results to the user */
	int ndf;
	int niter;
	double stdfit;
	double pvalue;
	double * dpar;
	double ** corel = NULL;
	bool covar_invalid = false;
	// restore original SIGINT function
	interrupt_setup();
	// tsm patchset 230: final progress report labels to console
	if(fit_verbosity == BRIEF)
		FitShowBrief(-2, chisq, chisq, P_A, lambda, STANDARD);
	// tsm patchset 230: final progress report to log file
	if(!fit_suppress_log) {
		if(fit_verbosity == VERBOSE)
			FitShow(iter, chisq, last_chisq, P_A, lambda, log_f);
		else
			FitShowBrief(iter, chisq, last_chisq, P_A, lambda, log_f);
	}
	// test covariance matrix
	if(covar) {
		for(i = 0; i < num_params; i++) {
			// diagonal elements must be larger than zero
			if(covar[i][i] <= 0.0) {
				// Not a fatal error, but prevent floating point exception later on
				Dblf2("Calculation error: non-positive diagonal element in covar. matrix of parameter '%s'.\n", par_name[i]);
				covar_invalid = true;
			}
		}
	}
	// HBB 970304: the maxiter patch:
	if((maxiter > 0) && (iter > maxiter)) {
		Dblf2("\nMaximum iteration count (%d) reached. Fit stopped.\n", maxiter);
	}
	else if(user_stop) {
		Dblf2("\nThe fit was stopped by the user after %d iterations.\n", iter);
	}
	else if(lambda >= MAX_LAMBDA) {
		Dblf2("\nThe maximum lambda = %e was exceeded. Fit stopped.\n", MAX_LAMBDA);
	}
	else if(covar_invalid) {
		Dblf2("\nThe covariance matrix is invalid. Fit did not converge properly.\n");
	}
	else {
		Dblf2("\nAfter %d iterations the fit converged.\n", iter);
		v = GpGg.Ev.AddUdvByName("FIT_CONVERGED");
		v->udv_value.SetInt(1);
	}
	// fit results
	ndf    = num_data - num_params;
	stdfit = sqrt(chisq / ndf);
	pvalue = 1. - chisq_cdf(ndf, chisq);
	niter = iter;
	// Export these to user-accessible variables
	v = GpGg.Ev.AddUdvByName("FIT_NDF");
	v->udv_value.SetInt(ndf);
	v = GpGg.Ev.AddUdvByName("FIT_STDFIT");
	v->udv_value.SetComplex(stdfit, 0);
	v = GpGg.Ev.AddUdvByName("FIT_WSSR");
	v->udv_value.SetComplex(chisq, 0);
	v = GpGg.Ev.AddUdvByName("FIT_P");
	v->udv_value.SetComplex(pvalue, 0);
	v = GpGg.Ev.AddUdvByName("FIT_NITER");
	v->udv_value.SetInt(niter);
	//
	// Save final parameters. Depending on the backend and
	// its internal state, the last call_gnuplot may not have been at the minimum
	//
	for(i = 0; i < num_params; i++)
		setvar(par_name[i], P_A[i] * scale_params[i]);
	// Set error and covariance variables to zero, thus making sure they are created.
	if(fit_errorvariables) {
		for(i = 0; i < num_params; i++)
			setvarerr(par_name[i], 0.0);
	}
	if(fit_covarvariables) {
		// first, remove all previous covariance variables
		GpGg.Ev.DelUdvByName("FIT_COV_*", true);
		for(i = 0; i < num_params; i++) {
			for(j = 0; j < i; j++) {
				setvarcovar(par_name[i], par_name[j], 0.0);
				setvarcovar(par_name[j], par_name[i], 0.0);
			}
			setvarcovar(par_name[i], par_name[i], 0.0);
		}
	}
	// calculate unscaled parameter errors in dpar[]:
	dpar = vec(num_params);
	if(covar && !covar_invalid) {
		// calculate unscaled parameter errors in dpar[]:
		for(i = 0; i < num_params; i++) {
			dpar[i] = sqrt(covar[i][i]);
		}
		// transform covariances into correlations
		corel = matr(num_params, num_params);
		for(i = 0; i < num_params; i++) {
			// only lower triangle needs to be handled
			for(j = 0; j < i; j++)
				corel[i][j] = covar[i][j] / (dpar[i] * dpar[j]);
			corel[i][i] = 1.;
		}
	}
	else {
		// set all errors to zero if covariance matrix invalid or unavailable
		for(i = 0; i < num_params; i++)
			dpar[i] = 0.0;
	}
	if(fit_errorscaling || (num_errors == 0)) {
		// scale parameter errors based on chisq
		const double temp = sqrt(chisq / (num_data - num_params));
		for(i = 0; i < num_params; i++)
			dpar[i] *= temp;
	}
	/* Save user error variables. */
	if(fit_errorvariables) {
		for(i = 0; i < num_params; i++)
			setvarerr(par_name[i], dpar[i] * scale_params[i]);
	}
	// fill covariance variables if needed
	if(fit_covarvariables && covar && !covar_invalid) {
		double scale = (fit_errorscaling || (num_errors == 0)) ? (chisq / (num_data - num_params)) : 1.0;
		for(i = 0; i < num_params; i++) {
			// only lower triangle needs to be handled
			for(j = 0; j <= i; j++) {
				const double temp = scale * scale_params[i] * scale_params[j];
				setvarcovar(par_name[i], par_name[j], covar[i][j] * temp);
				setvarcovar(par_name[j], par_name[i], covar[i][j] * temp);
			}
		}
	}
	ShowResults(chisq, last_chisq, P_A, dpar, corel);
	free(dpar);
	free_matr(corel);
}
//
// test for user request to stop the fit
//
//bool regress_check_stop(int iter, double chisq, double last_chisq, double lambda)
bool GpFit::RegressCheckStop(int iter, double chisq, double last_chisq, double lambda)
{
/*
 *  MSDOS defines signal(SIGINT) but doesn't handle it through
 *  real interrupts. So there remain cases in which a ctrl-c may
 *  be uncaught by signal. We must use kbhit() instead that really
 *  serves the keyboard interrupt (or write an own interrupt func
 *  which also generates #ifdefs)
 *
 *  I hope that other OSes do it better, if not... add #ifdefs :-(
 *  EMX does not have kbhit.
 *
 *  HBB: I think this can be enabled for DJGPP V2. SIGINT is actually
 *  handled there, AFAIK.
 */
#if(defined(MSDOS) && !defined(__EMX__))
	if(kbhit()) {
		do {
			getchx();
		} while(kbhit());
		ctrlc_flag = true;
	}
#endif
#ifdef WIN32
	/* This call makes the Windows GUI functional during fits.
	   Pressing Ctrl-Break now finally has an effect. */
	WinMessageLoop();
#endif
	if(GpGg.ctrlc_flag) {
		// Always report on current status
		if(fit_verbosity == VERBOSE)
			FitShow(iter, chisq, last_chisq, P_A, lambda, STANDARD);
		else
			FitShowBrief(iter, chisq, last_chisq, P_A, lambda, STANDARD);
		GpGg.ctrlc_flag = false;
		if(!FitInterrupt()) // handle keys
			return false;
	}
	return true;
}
//
// free memory allocated by gnuplot's internal fitting code
//
//static void internal_cleanup()
// static
void GpFit::InternalCleanup()
{
	free_matr(GpF.regress_C);
	GpF.regress_C = NULL;
	double lambda = -2; // flag value, meaning 'destruct!'
	GpF.Marquardt(NULL, NULL, NULL, &lambda);
}
//
// frame routine for the marquardt-fit
//
//static bool regress(double a[])
bool GpFit::Regress(double a[])
{
	double ** covar, ** C, chisq, last_chisq, lambda;
	int iter;
	marq_res_t res;
	regress_cleanup = &GpFit::InternalCleanup;
	chisq = last_chisq = INFINITY;
	// the global copy to is accessible to error_ex, too
	regress_C = C = matr(num_data + num_params, num_params);
	lambda = -1;            /* use sign as flag */
	iter = 0;               /* iteration counter  */
	// Initialize internal variables and 1st chi-square check
	if((res = Marquardt(a, C, &chisq, &lambda)) == ML_ERROR)
		Eex("FIT: error occurred during fit");
	res = BETTER;
	fit_show_lambda = true;
	FitProgress(iter, chisq, chisq, a, lambda, STANDARD);
	if(!fit_suppress_log)
		FitProgress(iter, chisq, chisq, a, lambda, log_f);
	RegressInit();
	// MAIN FIT LOOP: do the regression iteration
	do {
		if(!RegressCheckStop(iter, chisq, last_chisq, lambda))
			break;
		if(res == BETTER) {
			iter++;
			last_chisq = chisq;
		}
		if((res = Marquardt(a, C, &chisq, &lambda)) == BETTER)
			FitProgress(iter, chisq, last_chisq, a, lambda, STANDARD);
	} while((res != ML_ERROR) && (lambda < MAX_LAMBDA) && (!maxiter || (iter <= maxiter)) && chisq && (res == WORSE || ((last_chisq - chisq) > (epsilon * chisq + epsilon_abs))));
	// tsm patchset 230: change to new convergence criterion
	//
	// fit done
	if(res == ML_ERROR)
		Eex("FIT: error occurred during fit");
	// compute errors in the parameters
	// get covariance-, correlation- and curvature-matrix and errors in the parameters
	// compute covar[][] directly from C
	Givens(C, 0, 0, num_data, num_params);
	// Use lower square of C for covar
	covar = C + num_data;
	Invert_RtR(C, covar, num_params);
	RegressFinalize(iter, chisq, last_chisq, lambda, covar);
	// call destructor for allocated vars
	GpFit::InternalCleanup();
	regress_cleanup = NULL;
	return true;
}
//
// display results of the fit
//
//static void show_results(double chisq, double last_chisq, double * a, double * dpar, double ** corel)
void GpFit::ShowResults(double chisq, double last_chisq, double * a, double * dpar, double ** corel)
{
	int i, j, k;
	bool have_errors = true;
	Dblf2("final sum of squares of residuals : %g\n", chisq);
	if(chisq > NEARLY_ZERO) {
		Dblf2("rel. change during last iteration : %g\n\n", (chisq - last_chisq) / chisq);
	}
	else {
		Dblf2("abs. change during last iteration : %g\n\n", (chisq - last_chisq));
	}
	if((num_data == num_params) && ((num_errors == 0) || fit_errorscaling)) {
		Dblf("\nExactly as many data points as there are parameters.\n");
		Dblf("In this degenerate case, all errors are zero by definition.\n\n");
		have_errors = false;
	}
	else if((chisq < NEARLY_ZERO) && ((num_errors == 0) || fit_errorscaling)) {
		Dblf("\nHmmmm.... Sum of squared residuals is zero. Can't compute errors.\n\n");
		have_errors = false;
	}
	else if(corel == NULL) {
		Dblf("\nCovariance matric unavailable. Can't compute errors.\n\n");
		have_errors = false;
	}
	if(!have_errors) {
		Dblf("Final set of parameters \n");
		Dblf("======================= \n\n");
		for(k = 0; k < num_params; k++)
			Dblf3("%-15.15s = %-15g\n", par_name[k], a[k] * scale_params[k]);
	}
	else {
		int ndf          = num_data - num_params;
		double stdfit    = sqrt(chisq/ndf);
		double pvalue    = 1. - chisq_cdf(ndf, chisq);
		Dblf2("degrees of freedom    (FIT_NDF)                        : %d\n", ndf);
		Dblf2("rms of residuals      (FIT_STDFIT) = sqrt(WSSR/ndf)    : %g\n", stdfit);
		Dblf2("variance of residuals (reduced chisquare) = WSSR/ndf   : %g\n", chisq / ndf);
		/* We cannot know if the errors supplied by the user are weighting factors
		   or real errors, so we print the p-value in any case, although it does not
		   make much sense in the first case.  This means that we print this for x:y:z:(1)
		   fits without errors using the old syntax since this requires 4 columns. */
		if(num_errors > 0)
			Dblf2("p-value of the Chisq distribution (FIT_P)              : %g\n", pvalue);
		Dblf("\n");
		if(fit_errorscaling || (num_errors == 0))
			Dblf("Final set of parameters            Asymptotic Standard Error\n");
		else
			Dblf("Final set of parameters            Standard Deviation\n");
		Dblf("=======================            ==========================\n");
		for(i = 0; i < num_params; i++) {
			const double temp = (fabs(a[i]) < NEARLY_ZERO) ? 0.0 : fabs(100.0 * dpar[i] / a[i]);
			Dblf6("%-15.15s = %-15g  %-3.3s %-12.4g (%.4g%%)\n", par_name[i], a[i] * scale_params[i], "+/-", dpar[i] * scale_params[i], temp);
		}
		/* Print correlation matrix only if there is more than one parameter. */
		if((num_params > 1) && (corel != NULL)) {
			Dblf("\ncorrelation matrix of the fit parameters:\n");
			Dblf("                ");
			for(j = 0; j < num_params; j++)
				Dblf2("%-6.6s ", par_name[j]);
			Dblf("\n");
			for(i = 0; i < num_params; i++) {
				Dblf2("%-15.15s", par_name[i]);
				for(j = 0; j <= i; j++) {
					Dblf2("%6.3f ", corel[i][j]); /* Only print lower triangle of symmetric matrix */
				}
				Dblf("\n");
			}
		}
	}
}
//
// display actual state of the fit
//
//void fit_progress(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device)
void GpFit::FitProgress(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device)
{
	if(fit_verbosity == VERBOSE)
		FitShow(i, chisq, last_chisq, a, lambda, device);
	else if(fit_verbosity == BRIEF)
		FitShowBrief(i, chisq, last_chisq, a, lambda, device);
}

//static void fit_show(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device)
void GpFit::FitShow(int i, double chisq, double last_chisq, double* a, double lambda, FILE * device)
{
	fprintf(device, "\n\nIteration %d\nWSSR        : %-15g   delta(WSSR)/WSSR   : %g\ndelta(WSSR) : %-15g   limit for stopping : %g\n"                                                                                      ,
	    i, chisq, (chisq > NEARLY_ZERO) ? (chisq - last_chisq) / chisq : 0.0, chisq - last_chisq, epsilon);
	if(fit_show_lambda)
		fprintf(device, "lambda	     : %g\n", lambda);
	fprintf(device, "\n%s parameter values\n\n", (i > 0 ? "resultant" : "initial set of free"));
	for(int k = 0; k < num_params; k++)
		fprintf(device, "%-15.15s = %g\n", par_name[k], a[k] * scale_params[k]);
}

/* If the exponent of a floating point number in scientific format (%e) has three
   digits and the highest digit is zero, it will get removed by this routine. */
static char * pack_float(char * num)
{
	static int needs_packing = -1;
	if(needs_packing < 0) {
		// perform the test only once
		char buf[12];
		snprintf(buf, sizeof(buf), "%.2e", 1.00); /* "1.00e+000" or "1.00e+00" */
		needs_packing = (strlen(buf) == 9);
	}
	if(needs_packing) {
		char * p = strchr(num, 'e');
		if(p == NULL)
			p = strchr(num, 'E');
		if(p) {
			p += 2; /* also skip sign of exponent */
			if(*p == '0') {
				do {
					*p = *(p + 1);
				} while(*++p != NUL);
			}
		}
	}
	return num;
}
//
// tsm patchset 230: new one-line version of progress report
//
//static void fit_show_brief(int iter, double chisq, double last_chisq, double* parms, double lambda, FILE * device)
void GpFit::FitShowBrief(int iter, double chisq, double last_chisq, double* parms, double lambda, FILE * device)
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
		if(fit_show_lambda)
			strcat(buf, " lambda  ");
		// 1.00e+00
		fputs(buf, device);
		len = strlen(buf);
		for(k = 0; k < num_params; k++) {
			snprintf(buf, sizeof(buf), " %-13.13s", par_name[k]);
			len += strlen(buf);
			if((fit_wrap > 0) && (len >= fit_wrap)) {
				fprintf(device, "\n%*c", indent, ' ');
				len = indent;
			}
			fputs(buf, device);
		}
		fputs("\n", device);
	}
	// on iteration -2, don't print anything else
	if(iter == -2)
		return;
	// new convergence test quantities
	delta = chisq - last_chisq;
	lim = epsilon * chisq + epsilon_abs;
	// print values
	if(iter >= 0)
		snprintf(buf, sizeof(buf), "%4i", iter);
	else // -1 indicates that chisquare increased
		snprintf(buf, sizeof(buf), "%4c", '*');
	snprintf(buf + 4, sizeof(buf) - 4, " %-17.10e %- 10.2e", chisq, delta / lim);
	if(fit_show_lambda)
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %-9.2e", lambda);
	for(k = 0, p = buf + 4; (k < 3) && (p != NULL); k++) {
		p++;
		pack_float(p);
		p = strchr(p, 'e');
	}
	fputs(buf, device);
	len = strlen(buf);
	for(k = 0; k < num_params; k++) {
		snprintf(buf, sizeof(buf), " % 14.6e", parms[k] * scale_params[k]);
		pack_float(buf);
		len += strlen(buf);
		if((fit_wrap > 0) && (len >= fit_wrap)) {
			fprintf(device, "\n%*c", indent, ' ');
			len = indent;
		}
		fputs(buf, device);
	}
	fputs("\n", device);
}

/*****************************************************************
    is_empty: check for valid string entries
*****************************************************************/
static bool is_empty(char * s)
{
	while(*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
		s++;
	return (bool)(*s == '#' || *s == '\0');
}

/*****************************************************************
    get next word of a multi-word string, advance pointer
*****************************************************************/
static char * get_next_word(char ** s, char * subst)
{
	char * tmp = *s;
	while(*tmp == ' ' || *tmp == '\t' || *tmp == '=')
		tmp++;
	if(*tmp == '\n' || *tmp == '\r' || *tmp == '\0') /* not found */
		return NULL;
	if((*s = strpbrk(tmp, " =\t\n\r")) == NULL)
		*s = tmp + strlen(tmp);
	*subst = **s;
	*(*s)++ = '\0';
	return tmp;
}
//
// first time settings
//
void GpFit::Init()
{
	func.at = (AtType*)NULL; /* need to parse 1 time */
}
//
// Set a GNUPLOT user-defined variable
//
static void setvar(char * varname, double data)
{
	// Despite its name it is actually usable for any variable.
	GpGg.Ev.FillGpValFloat(varname, data);
}

/*****************************************************************
    Set a user-defined variable from an error variable:
    Take the parameter name, turn it  into an error parameter
    name (e.g. a to a_err) and then set it.
 ******************************************************************/
static void setvarerr(char * varname, double value)
{
	// Create the variable name by appending _err
	char * pErrValName = (char*)malloc(strlen(varname) + 6);
	sprintf(pErrValName, "%s_err", varname);
	setvar(pErrValName, value);
	free(pErrValName);
}

/*****************************************************************
    Set a user-defined covariance variable:
    Take the two parameter names, turn them into an covariance
    parameter name (e.g. a and b to FIT_COV_a_b) and then set it.
 ******************************************************************/
static void setvarcovar(char * varname1, char * varname2, double value)
{
	/* The name of the (new) covariance variable */
	char * pCovValName = (char*)malloc(strlen(varname1) + strlen(varname2) + 10);
	sprintf(pCovValName, "FIT_COV_%s_%s", varname1, varname2);
	setvar(pCovValName, value);
	free(pCovValName);
}

/*****************************************************************
    Get integer variable value
*****************************************************************/
static int getivar(const char * varname)
{
	UdvtEntry * v = GpGg.Ev.GetUdvByName(varname);
	return (v && (v->udv_value.type != NOTDEFINED)) ? real_int(&(v->udv_value)) : 0;
}

/*****************************************************************
    Get double variable value
*****************************************************************/
static double getdvar(const char * varname)
{
	UdvtEntry * v = GpGg.Ev.GetUdvByName(varname);
	return (v && (v->udv_value.type != NOTDEFINED)) ? v->udv_value.Real() : 0;
}

/*****************************************************************
   like getdvar, but
   - create it and set to `value` if not found or undefined
   - convert it from integer to real if necessary
*****************************************************************/
static double createdvar(char * varname, double value)
{
	UdvtEntry * udv_ptr = GpGg.Ev.AddUdvByName((char*)varname);
	if(udv_ptr->udv_value.type == NOTDEFINED) { /* new variable */
		udv_ptr->udv_value.SetComplex(value, 0.0);
	}
	else if(udv_ptr->udv_value.type == INTGR) { /* convert to CMPLX */
		udv_ptr->udv_value.SetComplex((double)udv_ptr->udv_value.v.int_val, 0.0);
	}
	return udv_ptr->udv_value.Real();
}

// argument: char *fn
#define VALID_FILENAME(fn) ((fn) != NULL && (*fn) != '\0')
//
// write the actual parameters to start parameter file
//
//void update(char * pfile, char * npfile)
void GpFit::Update(char * pfile, char * npfile)
{
	char ifilename[PATH_MAX];
	char * ofilename;
	bool createfile = false;
	if(existfile(pfile)) {
		/* update pfile npfile:
		   if npfile is a valid file name, take pfile as input file and
		   npfile as output file
		 */
		if(VALID_FILENAME(npfile)) {
			strnzcpy(ifilename, pfile, sizeof(ifilename));
			ofilename = npfile;
		}
		else {
#ifdef BACKUP_FILESYSTEM
			/* filesystem will keep original as previous version */
			strnzcpy(ifilename, pfile, sizeof(ifilename));
#else
			BackupFile(ifilename, pfile); /* will Eex if it fails */
#endif
			ofilename = pfile;
		}
	}
	else {
		// input file does not exists; will create new file
		createfile = true;
		ofilename = VALID_FILENAME(npfile) ? npfile : pfile;
	}
	if(createfile) {
		// The input file does not exists and--strictly speaking--there is
		// nothing to 'update'.  Instead of bailing out we guess the intended use:
		// We output all INTGR/CMPLX user variables and mark them as '# FIXED' if
		// they were not used during the last fit command.
		UdvtEntry * udv = GpGg.Ev.first_udv;
		FILE * nf;
		if((last_fit_command == NULL) || (strlen(last_fit_command) == 0)) {
			// Technically, a prior fit command isn't really required.  But since
			// all variables in the parameter file would be marked '# FIXED' in that
			// case, it cannot be directly used in a subsequent fit command.
#if 1
			Eex2("'update' requires a prior 'fit' since the parameter file %s does not exist yet.", ofilename);
#else
			fprintf(stderr, "'update' without a prior 'fit' and without a previous parameter file:\n");
			fprintf(stderr, " all variables will be marked '# FIXED'!\n");
#endif
		}
		if(!(nf = fopen(ofilename, "w")))
			Eex2("new parameter file %s could not be created", ofilename);
		fputs("# Parameter file created by 'update' from current variables\n", nf);
		if((last_fit_command != NULL) && (strlen(last_fit_command) > 0))
			fprintf(nf, "## %s\n", last_fit_command);
		while(udv) {
			if((strncmp(udv->udv_name, "GPVAL_", 6) == 0) || (strncmp(udv->udv_name, "MOUSE_", 6) == 0) ||
			    (strncmp(udv->udv_name, "FIT_", 4) == 0) || (strcmp(udv->udv_name, "NaN") == 0) || (strcmp(udv->udv_name, "pi") == 0)) {
				// skip GPVAL_, MOUSE_, FIT_ and builtin variables
				udv = udv->next_udv;
				continue;
			}
			if((udv->udv_value.type == INTGR) || (udv->udv_value.type == CMPLX)) {
				int k;
				// ignore indep. variables
				for(k = 0; k < MAX_NUM_VAR; k++) {
					if(last_dummy_var[k] && strcmp(last_dummy_var[k], udv->udv_name) == 0)
						break;
				}
				if(k != MAX_NUM_VAR) {
					udv = udv->next_udv;
					continue;
				}
				if(udv->udv_value.type == INTGR)
					fprintf(nf, "%-15s = %-22i", udv->udv_name, udv->udv_value.v.int_val);
				else /* CMPLX */
					fprintf(nf, "%-15s = %-22s", udv->udv_name, num_to_str(udv->udv_value.v.cmplx_val.real));
				// mark variables not used for the last fit as fixed
				for(k = 0; k < last_num_params; k++) {
					if(strcmp(last_par_name[k], udv->udv_name) == 0)
						break;
				}
				if(k == last_num_params)
					fprintf(nf, "   %s", GP_FIXED);
				putc('\n', nf);
			}
			udv = udv->next_udv;
		}
		if(fclose(nf))
			Eex("I/O error during update");
	}
	else { // !createfile
		// input file exists - this is the originally intended case of
		// the update command: update an existing parameter file
		char sstr[256];
		char * s = sstr;
		char * fnam;
		FILE * of, * nf;
		if(!(of = loadpath_fopen(ifilename, "r")))
			Eex2("parameter file %s could not be read", ifilename);
		if(!(nf = fopen(ofilename, "w")))
			Eex2("new parameter file %s could not be created", ofilename);
		fnam = gp_basename(ifilename); /* strip off the path */
		SETIFZ(fnam, ifilename);
		while(fgets(s = sstr, sizeof(sstr), of) != NULL) {
			char pname[64]; /* name of parameter */
			double pval; /* parameter value */
			char tail[127]; /* trailing characters */
			char * tmp;
			char c;
			if(is_empty(s)) {
				fputs(s, nf); /* preserve comments */
				continue;
			}
			if((tmp = strchr(s, '#')) != NULL) {
				strnzcpy(tail, tmp, sizeof(tail));
				*tmp = NUL;
			}
			else
				strcpy(tail, "\n");
			tmp = get_next_word(&s, &c);
			if(!legal_identifier(tmp) || strlen(tmp) > MAX_ID_LEN) {
				fclose(nf);
				fclose(of);
				Eex2("syntax error in parameter file %s", fnam);
			}
			strnzcpy(pname, tmp, sizeof(pname));
			// next must be '='
			if(c != '=') {
				tmp = strchr(s, '=');
				if(tmp == NULL) {
					fclose(nf);
					fclose(of);
					Eex2("syntax error in parameter file %s", fnam);
				}
				s = tmp + 1;
			}
			tmp = get_next_word(&s, &c);
			if(!sscanf(tmp, "%lf", &pval)) {
				fclose(nf);
				fclose(of);
				Eex2("syntax error in parameter file %s", fnam);
			}
			if((tmp = get_next_word(&s, &c)) != NULL) {
				fclose(nf);
				fclose(of);
				Eex2("syntax error in parameter file %s", fnam);
			}
			// now modify
			pval = getdvar(pname);
			fprintf(nf, "%-15s = %-22s   %s", pname, num_to_str(pval), tail);
		}
		if(fclose(nf) || fclose(of))
			Eex("I/O error during update");
	}
}

/*****************************************************************
    Backup a file by renaming it to something useful. Return
    the new name in tofile
*****************************************************************/

/* tofile must point to a char array[] or allocated data. See update() */

//static void backup_file(char * tofile, const char * fromfile)
void GpFit::BackupFile(char * tofile, const char * fromfile)
{
#if defined(MSDOS) || defined(VMS)
	char * tmpn;
#endif

/* first attempt, for all o/s other than MSDOS */

#ifndef MSDOS
	strcpy(tofile, fromfile);
#ifdef VMS
	/* replace all dots with _, since we will be adding the only
	 * dot allowed in VMS names
	 */
	while((tmpn = strchr(tofile, '.')) != NULL)
		*tmpn = '_';
#endif /*VMS */
	strcat(tofile, BACKUP_SUFFIX);
	if(rename(fromfile, tofile) == 0)
		return;         /* hurrah */
	if(existfile(tofile))
		Eex2("The backup file %s already exists and will not be overwritten.", tofile);
#endif

#ifdef MSDOS
	/* first attempt for msdos. */

	/* Copy only the first 8 characters of the filename, to comply
	 * with the restrictions of FAT filesystems. */
	strnzcpy(tofile, fromfile, 8 + 1);
	while((tmpn = strchr(tofile, '.')) != NULL)
		*tmpn = '_';
	strcat(tofile, BACKUP_SUFFIX);
	if(rename(fromfile, tofile) == 0)
		return;         /* success */
#endif /* MSDOS */

	/* get here => rename failed. */
	Eex3("Could not rename file %s to %s", fromfile, tofile);
}

// Modified from save.c:save_range()
static void log_axis_restriction(FILE * log_f, int param, double min, double max, int autoscale, char * name)
{
	char s[80];
	// FIXME: Is it really worth it to format time values?
	GpAxis * axis = (param == 1) ? &GpGg.GetY() : &GpGg.GetX();
	fprintf(log_f, "        %s range restricted to [", name);
	if(autoscale & AUTOSCALE_MIN) {
		putc('*', log_f);
	}
	else if(param < 2 && axis->datatype == DT_TIMEDATE) {
		putc('"', log_f);
		gstrftime(s, 80, GpGg.P_TimeFormat, min);
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
		gstrftime(s, 80, GpGg.P_TimeFormat, max);
		fputs(s, log_f);
		putc('"', log_f);
	}
	else {
		fprintf(log_f, "%#g", max);
	}
	fputs("]\n", log_f);
}

/*****************************************************************
    Recursively print definitions of function referenced.
*****************************************************************/
static int print_function_definitions_recursion(AtType * at, int * count, int maxfun, char * definitions[], int depth, int maxdepth)
{
	int rc = 0;
	if(at->a_count == 0)
		rc = 0;
	else if(*count == maxfun) /* limit the maximum number of unique function definitions  */
		rc = 1;
	else if(depth >= maxdepth) /* limit the maximum recursion depth */
		rc = 2;
	else {
		for(int i = 0; (i < at->a_count) && (*count < maxfun); i++) {
			if(oneof2(at->actions[i].Index, CALL, CALLN) && at->actions[i].arg.udf_arg->definition) {
				for(int k = 0; k < maxfun; k++) {
					if(definitions[k] == at->actions[i].arg.udf_arg->definition)
						break;  /* duplicate definition already in list */
					if(definitions[k] == NULL) {
						*count += 1; /* increment counter */
						definitions[k] = at->actions[i].arg.udf_arg->definition;
						break;
					}
				}
				rc |= print_function_definitions_recursion(at->actions[i].arg.udf_arg->at, count, maxfun, definitions, depth + 1, maxdepth);
			}
		}
	}
	return rc;
}

static void print_function_definitions(AtType * at, FILE * device)
{
	char * definitions[32];
	const int maxfun   = 32; /* maximum number of unique functions definitions */
	const int maxdepth = 20; /* maximum recursion depth */
	int count = 0;
	int k, rc;
	memzero(definitions, maxfun * sizeof(char *));
	rc = print_function_definitions_recursion(at, &count, maxfun, definitions, 0, maxdepth);
	for(k = 0; k < count; k++)
		fprintf(device, "\t%s\n", definitions[k]);
	if((rc & 1) != 0)
		fprintf(device, "\t[omitting further function definitions (max=%i)]\n", maxfun);
	if((rc & 2) != 0)
		fprintf(device, "\t[too many nested (or recursive) function definitions (max=%i)]\n", maxdepth);
}

/*****************************************************************
    Interface to the gnuplot "fit" command
*****************************************************************/

//void fit_command()
void GpFit::FitCommand(GpCommand & rC)
{
	Implement_FitCommand(rC);
}

void GpFit::Implement_FitCommand(GpCommand & rC)
{
	// HBB 20000430: revised this completely, to make it more similar to
	// what plot3drequest() does
	// Backwards compatibility - these were the default names in 4.4 and 4.6
	static const char * dummy_old_default[5] = {"x", "y", "t", "u", "v"};
	// Keep range info in local storage rather than overwriting axis structure.
	// The final range is "z" (actually the range of the function value).
	double range_min[MAX_NUM_VAR+1];
	double range_max[MAX_NUM_VAR+1];
	int range_autoscale[MAX_NUM_VAR+1];
	int num_ranges = 0;
	int max_data;
	int max_params;
	int dummy_token[MAX_NUM_VAR+1]; // Point to variable name for each explicit range
	int skipped[MAX_NUM_VAR+1];     // number of points out of range
	int num_points = 0;             // number of data points read from file
	static const int iz = MAX_NUM_VAR;
	int i, j;
	double v[MAX_NUM_VAR+2];
	double tmpd;
	time_t timer;
	int token1, token2, token3;
	char * tmp, * file_name;
	bool zero_initial_value;
	GpAxis * fit_xaxis, * fit_yaxis, * fit_zaxis;

	GpGg.XAxis = FIRST_X_AXIS;
	GpGg.YAxis = FIRST_Y_AXIS;
	GpGg.ZAxis = FIRST_Z_AXIS;
	fit_xaxis = &GpGg[FIRST_X_AXIS];
	fit_yaxis = &GpGg[FIRST_Y_AXIS];
	fit_zaxis = &GpGg[FIRST_Z_AXIS];

	rC.CToken++;
	//
	// First look for a restricted fit range...
	// Start with the current range limits on variable 1 ("x"),
	// variable 2 ("y"), and function range ("z").
	// Historically variables 3-5 inherited the current range of t, u, and v
	// but no longer.  NB: THIS IS A CHANGE
	// 
	fit_xaxis->Init(0);
	fit_yaxis->Init(0);
	fit_zaxis->Init(1);
	for(i = 0; i < MAX_NUM_VAR+1; i++)
		dummy_token[i] = -1;
	range_min[0] = fit_xaxis->Range.low;
	range_max[0] = fit_xaxis->Range.upp;
	range_autoscale[0] = fit_xaxis->AutoScale;
	range_min[1] = fit_yaxis->Range.low;
	range_max[1] = fit_yaxis->Range.upp;
	range_autoscale[1] = fit_yaxis->AutoScale;
	for(i = 2; i < MAX_NUM_VAR; i++) {
		range_min[i] = GPVL;
		range_max[i] = -GPVL;
		range_autoscale[i] = AUTOSCALE_BOTH;
	}
	range_min[iz] = fit_zaxis->Range.low;
	range_max[iz] = fit_zaxis->Range.upp;
	range_autoscale[iz] = fit_zaxis->AutoScale;

	num_ranges = 0;
	while(rC.Eq("[")) {
		GpAxis * scratch_axis = &GpGg[SAMPLE_AXIS];
		if(i > MAX_NUM_VAR)
			Eexc(rC.CToken, "too many range specifiers");
		scratch_axis->Init(1);
		dummy_token[num_ranges] = GpGg.ParseRange((AXIS_INDEX)scratch_axis->Index, rC);
		range_min[num_ranges] = scratch_axis->Range.low;
		range_max[num_ranges] = scratch_axis->Range.upp;
		range_autoscale[num_ranges] = scratch_axis->AutoScale;
		num_ranges++;
	}
	// now compile the function
	token1 = rC.CToken;
	if(func.at) {
		AtType::Destroy(func.at);
		func.at = NULL; // in case perm_at() does int_error
	}
	rC.P_DummyFunc = &func;
	// set all possible dummy variable names, even if we're using fewer
	for(i = 0; i < MAX_NUM_VAR; i++) {
		if(dummy_token[i] > 0)
			rC.CopyStr(rC.P.CDummyVar[i], dummy_token[i], MAX_ID_LEN);
		else if(*rC.P.SetDummyVar[i] != '\0')
			strcpy(rC.P.CDummyVar[i], rC.P.SetDummyVar[i]);
		else if(i < 5) /* Fall back to legacy ordering x y t u v */
			strcpy(rC.P.CDummyVar[i], dummy_old_default[i]);
		fit_dummy_udvs[i] = GpGg.Ev.AddUdvByName(rC.P.CDummyVar[i]);
	}
	memzero(rC.P.FitDummyVar, sizeof(rC.P.FitDummyVar));
	func.at = rC.P.PermAt(); // parse expression and save action table
	rC.P_DummyFunc = NULL;
	token2 = rC.CToken;
	// get filename
	file_name = rC.P.StringOrExpress(rC, NULL);
	if(file_name)
		file_name = gp_strdup(file_name);
	else
		Eexc(token2, "missing filename or datablock");
	// use datafile module to parse the datafile and qualifiers
	GpDf.DfSetPlotMode(MODE_QUERY); // Does nothing except for binary datafiles

	/* Historically we could only handle 7 using specs, hence 5 independent	*/
	/* variables (the last 2 cols are used for z and z_err).			*/
	/* June 2013 - Now the number of using specs can be increased by changing	*/
	/* MAXDATACOLS.  Logically this should be at least as large as MAX_NUM_VAR,	*/
	/* the limit on parameters passed to a user-defined function.		*/
	/* I.e. we expect that MAXDATACOLS >= MAX_NUM_VAR + 2			*/
	columns = GpDf.DfOpen(rC, file_name, MAX_NUM_VAR+2, NULL);
	if(columns < 0)
		Eexc2(token2, "Can't read data from", file_name);
	free(file_name);
	if(columns == 1)
		Eexc(rC.CToken, "Need more than 1 input data column");
	// Allow time data only on first two dimensions (x and y)
	GpDf.df_axis[0] = FIRST_X_AXIS;
	GpDf.df_axis[1] = FIRST_Y_AXIS;
	// BM: New options to distinguish fits with and without errors
	memzero(err_cols, sizeof(err_cols)); // reset error columns
	if(rC.AlmostEq("err$ors")) {
		/* error column specs follow */
		rC.CToken++;
		num_errors = 0;
		do {
			char * err_spec = NULL;
			if(!rC.IsLetter(rC.CToken))
				Eexc(rC.CToken, "Expecting a variable specifier.");
			rC.MCapture(&err_spec, rC.CToken, rC.CToken);
			/* check if this is a valid dummy var */
			for(i = 0; i < MAX_NUM_VAR; i++) {
				if(strcmp(err_spec, rC.P.CDummyVar[i]) == 0) {
					err_cols[i] = true;
					num_errors++;
					break;
				}
			}
			if(i == MAX_NUM_VAR) { /* variable name not found, yet */
				if(strcmp(err_spec, "z") == 0) {
					err_cols[iz] = true;
					num_errors++;
				}
				else
					Eexc(rC.CToken, "Invalid variable specifier.");
			}
			FPRINTF((stderr, "error spec \"%s\"\n", err_spec));
			free(err_spec);
		} while(rC.Eq(++rC.CToken, ",") && ++rC.CToken);
		// z-errors are required.
		if(!err_cols[iz]) {
			Eexc(rC.CToken, "z-errors are required.");
			err_cols[iz] = true;
			num_errors++;
		}
		// The dummy variable with the highest index indicates the minimum number of indep. variables required.
		num_indep = 0;
		for(i = 0; i < MAX_NUM_VAR; i++) {
			if(err_cols[i])
				num_indep = i + 1;
		}
		// Check if there are enough columns. Require # of indep. and dependent variables + # of errors
		if((columns != 0) && (columns < num_indep + 1 + num_errors))
			Eexc2(rC.CToken, "Not enough columns in using spec.  At least %i are required for this error spec.", num_indep + 1 + num_errors);
		/* Success. */
		if(columns > 0)
			num_indep = columns - num_errors - 1;
	}
	else if(rC.AlmostEq("zerr$ors")) {
		/* convenience alias */
		if(columns == 1)
			Eexc(rC.CToken, "zerror requires at least 2 columns");
		num_indep = (columns == 0) ? 1 : columns - 2;
		num_errors = 1;
		err_cols[iz] = true;
		rC.CToken++;
	}
	else if(rC.AlmostEq("yerr$ors")) {
		/* convenience alias, x:z:sz (or x:y:sy) */
		if((columns != 0) && (columns != 3))
			Eexc(rC.CToken, "yerror requires exactly 3 columns");
		num_indep = 1;
		num_errors = 1;
		err_cols[iz] = true;
		rC.CToken++;
	}
	else if(rC.AlmostEq("xyerr$ors")) {
		/* convienience alias, x:z:sx:sz (or x:y:sx:sy) */
		if((columns != 0) && (columns != 4))
			Eexc(rC.CToken, "xyerror requires exactly 4 columns");
		num_indep = 1;
		num_errors = 2;
		err_cols[0] = true;
		err_cols[iz] = true;
		rC.CToken++;
	}
	else if(rC.AlmostEq("uni$tweights")) {
		// 'unitweights' are the default now. So basically this option is only useful in v4 compatibility mode
		// no error columns given
		rC.CToken++;
		num_indep = (columns == 0) ? 1 : columns - 1;
		num_errors = 0;
	}
	else {
		/* no error keyword found */
		if(fit_v4compatible) {
			/* using old syntax */
			num_indep = (columns < 3) ? 1 : columns - 2;
			num_errors = (columns < 3) ? 0 : 1;
			if(num_errors > 0)
				err_cols[iz] = true;
		}
		else if(columns >= 3 && rC.P.FitDummyVar[columns-2] == 0) {
			int_warn(NO_CARET,
			    "\n\t> Implied independent variable %s not found in fit function."
			    "\n\t> Assuming version 4 syntax with zerror in column %d but no zerror keyword.\n",
			    rC.P.CDummyVar[columns-2], columns);
			num_indep = columns - 2;
			num_errors = 1;
			err_cols[iz] = true;
		}
		else {
			/* default to unitweights */
			num_indep = (columns == 0) ? 1 : columns - 1;
			num_errors = 0;
		}
	}
	FPRINTF((stderr, "cmd=%s\n", rC.P_InputLine));
	FPRINTF((stderr, "cols=%i indep=%i errors=%i\n", columns, num_indep, num_errors));
	// HBB 980401: if this is a single-variable fit, we shouldn't have
	// allowed a variable name specifier for 'y':
	// FIXME EAM - Is this relevant any more?
	if((dummy_token[1] > 0) && (num_indep == 1))
		Eexc(dummy_token[1], "Can't re-name 'y' in a one-variable fit");
	// depending on number of independent variables, the last range spec may be for the Z axis
	if(num_ranges > num_indep+1)
		Eexc2(dummy_token[num_ranges-1], "Too many range-specs for a %d-variable fit", num_indep);
	if(num_ranges == (num_indep + 1)) {
		// last range was actually for the independen variable
		range_min[iz] = range_min[num_indep];
		range_max[iz] = range_max[num_indep];
		range_autoscale[iz] = range_autoscale[num_indep];
	}
	// defer actually reading the data until we have parsed the rest of the line
	token3 = rC.CToken;
	// open logfile before we use any Dblfn calls
	if(!fit_suppress_log) {
		char * p_logfile = GetLogfile();
		if(p_logfile && !log_f && !(log_f = fopen(p_logfile, "a")))
			Eex2("could not open log-file %s", p_logfile);
		free(p_logfile);
	}
	tmpd = getdvar(FITLIMIT); // get epsilon if given explicitly
	epsilon = (tmpd < 1.0 && tmpd > 0.0) ? tmpd : DEF_FIT_LIMIT;
	FPRINTF((STANDARD, "epsilon=%e\n", epsilon));
	// tsm patchset 230: new absolute convergence variable
	FPRINTF((STANDARD, "epsilon_abs=%e\n", epsilon_abs));
	// HBB 970304: maxiter patch
	maxiter = getivar(FITMAXITER);
	SETMAX(maxiter, 0);
	FPRINTF((STANDARD, "maxiter=%i\n", maxiter));
	// get startup value for lambda, if given
	tmpd = getdvar(FITSTARTLAMBDA);
	if(tmpd > 0.0) {
		startup_lambda = tmpd;
		Dblf2("lambda start value set: %g\n", startup_lambda);
	}
	else {
		startup_lambda = 0.0; // use default value or calculation
	}
	// get lambda up/down factor, if given
	tmpd = getdvar(FITLAMBDAFACTOR);
	if(tmpd > 0.0) {
		lambda_up_factor = lambda_down_factor = tmpd;
		Dblf2("lambda scaling factors reset:  %g\n", lambda_up_factor);
	}
	else {
		lambda_down_factor = LAMBDA_DOWN_FACTOR;
		lambda_up_factor = LAMBDA_UP_FACTOR;
	}
	FPRINTF((STANDARD, "prescale=%i\n", fit_prescale));
	FPRINTF((STANDARD, "errorscaling=%i\n", fit_errorscaling));
	(void)time(&timer);
	if(!fit_suppress_log) {
		char * line = NULL;
		fputs("\n\n*******************************************************************************\n", log_f);
		fprintf(log_f, "%s\n\n", ctime(&timer));
		rC.MCapture(&line, token2, token3 - 1);
		fprintf(log_f, "FIT:    data read from %s\n", line);
		fprintf(log_f, "        format = ");
		free(line);
		for(i = 0; (i < num_indep) && (i < columns - 1); i++)
			fprintf(log_f, "%s:", rC.P.CDummyVar[i]);
		fprintf(log_f, "z");
		if(num_errors > 0) {
			for(i = 0; (i < num_indep) && (i < columns - 1); i++)
				if(err_cols[i])
					fprintf(log_f, ":s%s", rC.P.CDummyVar[i]);
			fprintf(log_f, ":s\n");
		}
		else {
			fprintf(log_f, "\n");
		}
	}
	// report all range specs, starting with Z
	if(!fit_suppress_log) {
		if((range_autoscale[iz] & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
			log_axis_restriction(log_f, iz, range_min[iz], range_max[iz], range_autoscale[iz], "function");
		for(i = 0; i < num_indep; i++) {
			if((range_autoscale[i] & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
				log_axis_restriction(log_f, i, range_min[i], range_max[i], range_autoscale[i], rC.P.CDummyVar[i]);
		}
	}
	// start by allocting memory for MAX_DATA datapoints
	max_data = MAX_DATA;
	fit_x = vec(max_data * num_indep);
	fit_z = vec(max_data);
	// allocate error array, last one is always the z-error
	err_data = vec(max_data * MAX(num_errors, 1));
	num_data = 0;
	/* Set skipped[i] = 0 for all i */
	memzero(skipped, sizeof(skipped));
	/* first read in experimental data */

	/* If the user has set an explicit locale for numeric input, apply it */
	/* here so that it affects data fields read from the input file.      */
	set_numeric_locale();
	while((i = GpDf.DfReadLine(v, num_indep + num_errors + 1)) != DF_EOF) {
		if(num_data >= max_data) {
			// increase max_data by factor of 1.5
			max_data = (max_data * 3) / 2;
			if(0 || !redim_vec(&fit_x, max_data * num_indep) || !redim_vec(&fit_z, max_data) || !redim_vec(&err_data, max_data * MAX(num_errors, 1))) {
				/* Some of the reallocations went bad: */
				Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
			}
			else {
				// Just so we know that the routine is at work:
				fprintf(STANDARD, "Max. number of data points scaled up to: %d\n", max_data);
			}
		} // if(need to extend storage space)
		// BM: silently ignore lines with NaN
		{
			bool skip_nan = false;
			for(int k = 0; k < i; k++) {
				if(fisnan(v[k]))
					skip_nan = true;
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
			    Eex2("bad data on line %d of datafile", GpDf.df_line_number);
			    break;
			case 1: /* only z provided */
			    v[1] = v[0];
			    v[0] = (double)GpDf.df_datum;
			    break;
			default: /* June 2013 - allow more than 7 data columns */
			    if(i<0)
				    GpGg.IntErrorNoCaret("unexpected value returned by df_readline");
			    break;
		}
		num_points++;
		// skip this point if it is out of range
		for(i = 0; i < num_indep; i++) {
			if(!(range_autoscale[i] & AUTOSCALE_MIN) && (v[i] < range_min[i])) {
				skipped[i]++;
				goto out_of_range;
			}
			if(!(range_autoscale[i] & AUTOSCALE_MAX) && (v[i] > range_max[i])) {
				skipped[i]++;
				goto out_of_range;
			}
			fit_x[num_data * num_indep + i] = v[i]; /* save independent variable data */
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
		fit_z[num_data] = v[i++]; // save dependent variable data
		//
		// only use error from data file if _explicitly_ asked for by a using spec
		//
		if(num_errors == 0)
			err_data[num_data] = 1.0;  /* constant weight */
		else if(num_errors == 1)
			err_data[num_data] = v[i++];  /* z-error */
		else {
			int    idx = 0;
			for(int k = 0; k < MAX_NUM_VAR; k++) {
				if(err_cols[k])
					err_data[num_errors * num_data + idx++] = v[i++];
			}
			if(err_cols[iz])
				err_data[num_errors * num_data + idx] = v[i++];  /* z-error */
			else
				Eexc(NO_CARET, "z errors are always required"); // This case is not currently allowed. We always require z-errors.
		}
		// Increment index into stored values.
		// Note that out-of-range or NaN values bypass this operation.
		num_data++;
out_of_range:
		;
	}
	GpDf.DfClose();
	// We are finished reading user input; return to C locale for internal use
	reset_numeric_locale();
	if(num_data <= 1) {
		// no data! Try to explain why.
		printf("         Read %d points\n", num_points);
		for(i = 0; i < num_indep; i++) {
			if(skipped[i]) {
				printf("         Skipped %d points outside range [%s=", skipped[i], rC.P.CDummyVar[i]);
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
			if(GpGg[FIRST_Z_AXIS].AutoScale & AUTOSCALE_MIN)
				printf("*:");
			else
				printf("%g:", GpGg[FIRST_Z_AXIS].Range.low);
			if(GpGg[FIRST_Z_AXIS].AutoScale & AUTOSCALE_MAX)
				printf("*]\n");
			else
				printf("%g]\n", GpGg[FIRST_Z_AXIS].Range.upp);
		}
		Eex("No data to fit");
	}
	// tsm patchset 230: check for zero error values
	if(num_errors > 0) {
		for(i = 0; i < num_data; i++) {
			if(err_data[i * num_errors + (num_errors - 1)] == 0.0) {
				Dblf("\nCurrent data point\n");
				Dblf("=========================\n");
				Dblf3("%-15s = %i out of %i\n", "#", i + 1, num_data);
				for(j = 0; j < num_indep; j++)
					Dblf3("%-15.15s = %-15g\n", rC.P.CDummyVar[j], fit_x[i * num_indep + j]);
				Dblf3("%-15.15s = %-15g\n", "z", fit_z[i]);
				Dblf3("%-15.15s = %-15g\n", "s", err_data[i * num_errors + (num_errors - 1)]);
				Dblf("\n");
				Eex("Zero error value in data file");
			}
		}
	}
	// now resize fields to actual length:
	redim_vec(&fit_x, num_data * num_indep);
	redim_vec(&fit_z, num_data);
	redim_vec(&err_data, num_data * MAX(num_errors, 1));
	if(!fit_suppress_log) {
		char * line = NULL;
		fprintf(log_f, "        #datapoints = %d\n", num_data);
		if(num_errors == 0)
			fputs("        residuals are weighted equally (unit weight)\n\n", log_f);
		rC.MCapture(&line, token1, token2 - 1);
		fprintf(log_f, "function used for fitting: %s\n", line);
		print_function_definitions(func.at, log_f);
		free(line);
	}
	/* read in parameters */
	max_params = MAX_PARAMS; /* HBB 971023: make this resizeable */
	if(!rC.Eq(rC.CToken++, "via"))
		Eexc(rC.CToken, "Need via and either parameter list or file");
	// allocate arrays for parameter values, names
	P_A = vec(max_params);
	par_name = (fixstr*)malloc((max_params + 1) * sizeof(fixstr));
	num_params = 0;
	if(rC.IsStringValue(rC.CToken)) {    /* It's a parameter *file* */
		bool fixed;
		double tmp_par;
		char c, * s;
		char sstr[MAX_LINE_LEN + 1];
		FILE * f;
		static char * viafile = NULL;
		free(viafile);          /* Free previous name, if any */
		viafile = rC.TryToGetString();
		if(!viafile || !(f = loadpath_fopen(viafile, "r")))
			Eex2("could not read parameter-file \"%s\"", viafile);
		if(!fit_suppress_log)
			fprintf(log_f, "fitted parameters and initial values from file: %s\n\n", viafile);

		/* get parameters and values out of file and ignore fixed ones */

		while(true) {
			if(!fgets(s = sstr, sizeof(sstr), f)) /* EOF found */
				break;
			if((tmp = strstr(s, GP_FIXED)) != NULL) { /* ignore fixed params */
				*tmp = NUL;
				if(!fit_suppress_log)
					fprintf(log_f, "FIXED:  %s\n", s);
				fixed = true;
			}
			else
				fixed = false;
			tmp = strchr(s, '#');
			if(tmp)
				*tmp = NUL;
			if(is_empty(s))
				continue;
			tmp = get_next_word(&s, &c);
			if(!legal_identifier(tmp) || strlen(tmp) > MAX_ID_LEN) {
				fclose(f);
				Eex("syntax error in parameter file");
			}
			strnzcpy(par_name[num_params], tmp, sizeof(fixstr));
			/* next must be '=' */
			if(c != '=') {
				tmp = strchr(s, '=');
				if(tmp == NULL) {
					fclose(f);
					Eex("syntax error in parameter file");
				}
				s = tmp + 1;
			}
			tmp = get_next_word(&s, &c);
			if(sscanf(tmp, "%lf", &tmp_par) != 1) {
				fclose(f);
				Eex("syntax error in parameter file");
			}
			// make fixed params visible to GNUPLOT
			if(fixed) {
				setvar(par_name[num_params], tmp_par); // use parname as temp
			}
			else {
				if(num_params >= max_params) {
					max_params = (max_params * 3) / 2;
					if(0 || !redim_vec(&P_A, max_params) || !(par_name = (fixstr*)gp_realloc(par_name, (max_params + 1) * sizeof(fixstr), "fit param resize"))) {
						fclose(f);
						Eex("Out of memory in fit: too many parameters?");
					}
				}
				P_A[num_params++] = tmp_par;
			}
			if((tmp = get_next_word(&s, &c)) != NULL) {
				fclose(f);
				Eex2("syntax error in parameter file %s", viafile);
			}
		}
		fclose(f);
	}
	else {
		// not a string after via: it's a variable listing
		if(!fit_suppress_log)
			fputs("fitted parameters initialized with current variable values\n\n", log_f);
		do {
			if(!rC.IsLetter(rC.CToken))
				Eex("no parameter specified");
			rC.Capture(par_name[num_params], rC.CToken, rC.CToken, (int)sizeof(par_name[0]));
			if(num_params >= max_params) {
				max_params = (max_params * 3) / 2;
				if(0 || !redim_vec(&P_A, max_params) || !(par_name = (fixstr*)gp_realloc(par_name, (max_params + 1) * sizeof(fixstr), "fit param resize"))) {
					Eex("Out of memory in fit: too many parameters?");
				}
			}
			/* create variable if it doesn't exist */
			P_A[num_params] = createdvar(par_name[num_params], INITIAL_VALUE);
			++num_params;
		} while(rC.Eq(++rC.CToken, ",") && ++rC.CToken);
	}
	redim_vec(&P_A, num_params);
	par_name = (fixstr*)gp_realloc(par_name, (num_params + 1) * sizeof(fixstr), "fit param");
	if(num_data < num_params)
		Eex("Number of data points smaller than number of parameters");
	// initialize scaling parameters
	if(!redim_vec(&scale_params, num_params))
		Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
	zero_initial_value = false;
	for(i = 0; i < num_params; i++) {
		// avoid parameters being equal to zero
		if(P_A[i] == 0.0) {
			Dblf2("Warning: Initial value of parameter '%s' is zero.\n", par_name[i]);
			P_A[i] = NEARLY_ZERO;
			scale_params[i] = 1.0;
			zero_initial_value = true;
		}
		else if(fit_prescale) {
			// scale parameters, but preserve sign
			double a_sign = (P_A[i] > 0) - (P_A[i] < 0);
			scale_params[i] = a_sign * P_A[i];
			P_A[i] = a_sign;
		}
		else {
			scale_params[i] = 1.0;
		}
	}
	if(zero_initial_value) { /* print this message only once */
		/* tsm patchset 230: explain what good initial parameter values are */
		fprintf(STANDARD, "  Please provide non-zero initial values for the parameters, at least of\n");
		fprintf(STANDARD, "  the right order of magnitude. If the expected value is zero, then use\n");
		fprintf(STANDARD, "  the magnitude of the expected error. If all else fails, try 1.0\n\n");
	}
	if(num_params == 0)
		int_warn(NO_CARET, "No fittable parameters!\n");
	else
		Regress(P_A);  /* fit */
	SFile::ZClose(&log_f);
	ZFREE(fit_x);
	ZFREE(fit_z);
	ZFREE(err_data);
	ZFREE(P_A);
	if(func.at) {
		AtType::Destroy(func.at);       /* release perm. action table */
		func.at = (AtType*)NULL;
	}
	// remember parameter names for 'update'
	last_num_params = num_params;
	free(last_par_name);
	last_par_name = par_name;
	// remember names of indep. variables for 'update'
	for(i = 0; i < MAX_NUM_VAR; i++) {
		free(last_dummy_var[i]);
		last_dummy_var[i] = gp_strdup(rC.P.CDummyVar[i]);
	}
	// remember last fit command for 'save'
	free(last_fit_command);
	last_fit_command = _strdup(rC.P_InputLine);
	for(i = 0; i < rC.NumTokens; i++) {
		if(rC.Eq(i, ";")) {
			last_fit_command[rC.P_Token[i].start_index] = '\0';
			break;
		}
	}
	// save fit command to user variable
	GpGg.Ev.FillGpValString("GPVAL_LAST_FIT", last_fit_command);
}
//
// Print message to stderr and log file
//
void GpFit::Dblfn(const char * fmt, ...)
{
	va_list args;
	VA_START(args, fmt);
#if defined(HAVE_VFPRINTF) || _LIBC
	if(fit_verbosity != QUIET)
		vfprintf(STANDARD, fmt, args);
	va_end(args);
	if(!fit_suppress_log) {
		VA_START(args, fmt);
		vfprintf(log_f, fmt, args);
	}
#else
	if(fit_verbosity != QUIET)
		_doprnt(fmt, args, STANDARD);
	if(!fit_suppress_log) {
		_doprnt(fmt, args, log_f);
	}
#endif
	va_end(args);
}
//
// Get name of current log-file
//
//char * getfitlogfile()
char * GpFit::GetLogfile()
{
	char * logfile = NULL;
	if(fitlogfile == NULL) {
		char * tmp = getenv(GNUFITLOG); /* open logfile */
		if(!isempty(tmp)) {
			char * tmp2 = tmp + (strlen(tmp) - 1);
			// if given log file name ends in path separator, treat it as a directory to store the default "fit.log" in
			if(*tmp2 == '/' || *tmp2 == '\\') {
				logfile = (char*)malloc(strlen(tmp) + strlen(fitlogfile_default) + 1);
				strcpy(logfile, tmp);
				strcat(logfile, fitlogfile_default);
			}
			else {
				logfile = gp_strdup(tmp);
			}
		}
		else {
			logfile = gp_strdup(fitlogfile_default);
		}
	}
	else {
		logfile = gp_strdup(fitlogfile);
	}
	return logfile;
}

